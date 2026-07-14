/**
 * Part of AIA (AI Assistant) Subproject.
 *
 * Minimal `llama.cpp`-based CLI for loading a GGUF model and generating text
 * from a single prompt. This is the scaffolding step: no chat loop, no
 * sampling strategies beyond greedy decoding, no context persistence.
 *
 * @author  Gábor Krisztián Girhiny and Junie
 * @date    2026-07-14
 */

#include <libnova/log.hpp>
#include <libnova/error.hpp>
#include <libnova/main.hpp>

#include <llama.h>

#include <boost/program_options.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace po = boost::program_options;

const std::string LogTopic = "aia";

/**
 * @brief   System prompt steering the assistant towards asking clarifying
 *          questions and requesting missing context instead of guessing.
 *
 * This is the minimal step towards "intelligence": it doesn't change the
 * model, only how we frame the conversation for it.
 */
const std::string SystemPrompt =
    "You are a careful, precise assistant. If the user's request is ambiguous, "
    "underspecified, or depends on information you were not given (e.g. code, "
    "files, error messages, or requirements), do not guess: ask a short, specific "
    "clarifying question instead, or explicitly state what additional context you "
    "need. Only answer directly once you have enough context to be confident.";

/**
 * @brief   CLI options for the AIA tool.
 */
struct aia_options {
    std::string model_path {};
    std::string prompt {};
    std::string context_path {};
    int         n_predict { 128 };
    float       temperature { 0.8F };
    int         top_k { 40 };
    float       top_p { 0.95F };
    uint32_t    seed { LLAMA_DEFAULT_SEED };
    bool        help { false };
};

/**
 * @brief   Parse command line arguments.
 *
 * @param   args    Command line arguments (including argv[0]).
 *
 * @return  Parsed options, or `std::nullopt` on error.
 */
[[nodiscard]] auto parse_args(auto args) -> std::optional<aia_options> {
    std::vector<std::string> args_vec;
    for (const auto& arg : args | std::views::drop(1)) {
        args_vec.emplace_back(arg);
    }

    po::options_description desc("Options for aia");
    desc.add_options()
        ("help,h", "Produce help message")
        ("model,m", po::value<std::string>()->required(), "Path to a GGUF model file")
        ("prompt,p", po::value<std::string>()->required(), "Prompt to complete")
        ("context,c", po::value<std::string>(), "Path to a file whose contents are attached as context (e.g. code to review)")
        ("n-predict,n", po::value<int>()->default_value(128), "Number of tokens to generate")
        ("temperature,t", po::value<float>()->default_value(0.8F), "Sampling temperature (higher = more random)")
        ("top-k", po::value<int>()->default_value(40), "Keep only the top K most likely tokens (0 = disabled)")
        ("top-p", po::value<float>()->default_value(0.95F), "Keep the smallest set of tokens whose cumulative probability exceeds P (1.0 = disabled)")
        ("seed", po::value<uint32_t>()->default_value(LLAMA_DEFAULT_SEED), "Random seed for sampling (default: random)")
    ;

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(args_vec).options(desc).run(), vm);
        if (vm.contains("help")) {
            std::cout << "Usage: aia --model <model.gguf> --prompt \"<text>\" [--context <file>] [--n-predict <n>] "
                         "[--temperature <t>] [--top-k <k>] [--top-p <p>] [--seed <s>]\n";
            std::cout << desc << "\n";
            return aia_options { .help = true };
        }
        po::notify(vm);
    } catch (const std::exception& e) {
        nova::topic_log::error(LogTopic, "Error parsing arguments: {}", e.what());
        return std::nullopt;
    }

    return aia_options {
        .model_path   = vm["model"].as<std::string>(),
        .prompt       = vm["prompt"].as<std::string>(),
        .context_path = vm.contains("context") ? vm["context"].as<std::string>() : "",
        .n_predict    = vm["n-predict"].as<int>(),
        .temperature  = vm["temperature"].as<float>(),
        .top_k        = vm["top-k"].as<int>(),
        .top_p        = vm["top-p"].as<float>(),
        .seed         = vm["seed"].as<uint32_t>()
    };
}

/**
 * @brief   Read the contents of a context file, if given.
 *
 * @param   context_path    Path to the file, or empty for no context.
 *
 * @return  File contents, or `std::nullopt` if `context_path` is empty or the file could not be read.
 */
[[nodiscard]] auto read_context(const std::string& context_path) -> std::optional<std::string> {
    if (context_path.empty()) {
        return std::nullopt;
    }

    std::ifstream file(context_path);
    if (not file) {
        nova::topic_log::debug(LogTopic, "Could not open context file: {}", context_path);
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * @brief   Build the user-facing message, attaching context (e.g. file contents) if provided.
 *
 * @param   prompt      User's prompt text.
 * @param   context     Optional context to attach (e.g. code to review).
 *
 * @return  Combined user message.
 */
[[nodiscard]] auto build_user_message(const std::string& prompt, const std::optional<std::string>& context) -> std::string {
    if (not context) {
        return prompt;
    }

    return prompt + "\n\nContext:\n```\n" + *context + "\n```";
}

/**
 * @brief   Apply the model's chat template to a system + user message pair.
 *
 * Falls back to the raw user message if the model has no chat template.
 *
 * @param   model           Loaded model.
 * @param   system_prompt   System message content.
 * @param   user_message    User message content.
 *
 * @return  Formatted prompt ready for tokenization.
 */
[[nodiscard]] auto apply_chat_template(llama_model* model, const std::string& system_prompt, const std::string& user_message) -> std::string {
    const char* tmpl = llama_model_chat_template(model, nullptr);
    if (tmpl == nullptr) {
        nova::topic_log::debug(LogTopic, "Model has no chat template, falling back to raw prompt");
        return user_message;
    }

    const std::vector<llama_chat_message> messages {
        { "system", system_prompt.c_str() },
        { "user",   user_message.c_str() }
    };

    std::vector<char> buf(2 * (system_prompt.size() + user_message.size()) + 256);
    auto n_written = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, buf.data(), static_cast<int32_t>(buf.size()));
    if (n_written < 0) {
        nova::topic_log::debug(LogTopic, "Failed to apply chat template, falling back to raw prompt");
        return user_message;
    }

    if (static_cast<std::size_t>(n_written) > buf.size()) {
        buf.resize(static_cast<std::size_t>(n_written));
        n_written = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, buf.data(), static_cast<int32_t>(buf.size()));
    }

    return std::string(buf.data(), static_cast<std::size_t>(n_written));
}

/**
 * @brief   Tokenize a prompt string using the model's vocabulary.
 *
 * @param   vocab   Model vocabulary.
 * @param   prompt  Prompt text.
 *
 * @return  Token id sequence, including the beginning-of-sequence token if applicable.
 */
[[nodiscard]] auto tokenize_prompt(const llama_vocab* vocab, const std::string& prompt) -> std::vector<llama_token> {
    const auto n_tokens = -llama_tokenize(vocab, prompt.c_str(), static_cast<int>(prompt.size()), nullptr, 0, true, true);

    std::vector<llama_token> tokens(static_cast<std::size_t>(n_tokens));
    llama_tokenize(vocab, prompt.c_str(), static_cast<int>(prompt.size()), tokens.data(), static_cast<int>(tokens.size()), true, true);

    return tokens;
}

/**
 * @brief   Route `llama.cpp`/`ggml` internal log messages to the `aia` debug log.
 *
 * Registered via `llama_log_set` so that model loading, backend, and CUDA
 * diagnostics never pollute the normal (info) log, which is reserved for the
 * generated response.
 *
 * @param   level       `ggml` log level of the message.
 * @param   text        Log message text (may include a trailing newline).
 * @param   user_data   Unused.
 */
void ggml_log_to_topic(ggml_log_level level, const char* text, void* user_data) {
    (void)level;
    (void)user_data;

    std::string_view msg(text);
    while (not msg.empty() and (msg.back() == '\n' or msg.back() == '\r')) {
        msg.remove_suffix(1);
    }

    nova::topic_log::debug("ggml", "{}", msg);
}

/**
 * @brief   Build a sampler chain applying top-k, top-p, and temperature before final sampling.
 *
 * Chain order matters: top-k and top-p narrow the candidate set first, temperature
 * reshapes the remaining distribution, and `llama_sampler_init_dist` draws the final
 * token from it. This replaces plain greedy (argmax) decoding.
 *
 * @param   options CLI options carrying the sampling parameters.
 *
 * @return  Owning sampler chain; caller must `llama_sampler_free` it.
 */
[[nodiscard]] auto build_sampler(const aia_options& options) -> llama_sampler* {
    llama_sampler_chain_params sparams = llama_sampler_chain_default_params();
    llama_sampler* smpl = llama_sampler_chain_init(sparams);

    if (options.top_k > 0) {
        llama_sampler_chain_add(smpl, llama_sampler_init_top_k(options.top_k));
    }
    if (options.top_p < 1.0F) {
        llama_sampler_chain_add(smpl, llama_sampler_init_top_p(options.top_p, 1));
    }
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(options.temperature));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(options.seed));

    return smpl;
}

/**
 * @brief   Generate `n_predict` tokens continuing the given prompt and log the response.
 *
 * The prompt is wrapped with a system message (asking the model to request
 * clarification or context instead of guessing) and, if given, an attached
 * context (e.g. file contents), then formatted via the model's chat template.
 * Tokens are drawn via `smpl` (top-k/top-p/temperature sampling) instead of greedy argmax.
 *
 * @param   model       Loaded model.
 * @param   ctx         Inference context.
 * @param   smpl        Sampler chain used to pick each next token.
 * @param   prompt      Prompt text.
 * @param   context     Optional context to attach (e.g. code to review).
 * @param   n_predict   Number of tokens to generate.
 */
void generate(llama_model* model, llama_context* ctx, llama_sampler* smpl, const std::string& prompt, const std::optional<std::string>& context, int n_predict) {
    const llama_vocab* vocab = llama_model_get_vocab(model);

    const auto user_message = build_user_message(prompt, context);
    const auto formatted_prompt = apply_chat_template(model, SystemPrompt, user_message);

    nova::topic_log::debug(LogTopic, "Formatted prompt: {}", formatted_prompt);

    auto tokens = tokenize_prompt(vocab, formatted_prompt);

    // `llama_decode` requires the batch to fit within `n_batch`, so the (potentially
    // long, e.g. with attached context) prompt must be fed in chunks rather than as
    // a single oversized batch. Only the final chunk's logits are needed to start
    // sampling the continuation.
    const auto n_batch = llama_n_batch(ctx);
    std::size_t n_fed = 0;
    llama_batch batch {};
    while (n_fed < tokens.size()) {
        const auto chunk_size = std::min(static_cast<std::size_t>(n_batch), tokens.size() - n_fed);
        batch = llama_batch_get_one(tokens.data() + n_fed, static_cast<int>(chunk_size));
        if (llama_decode(ctx, batch) != 0) {
            nova::topic_log::error(LogTopic, "llama_decode failed while processing prompt");
            return;
        }
        n_fed += chunk_size;
    }

    std::string response;

    for (int i = 0; i < n_predict; ++i) {
        if (i > 0 and llama_decode(ctx, batch) != 0) {
            nova::topic_log::error(LogTopic, "llama_decode failed");
            return;
        }

        const llama_token next_token = llama_sampler_sample(smpl, ctx, batch.n_tokens - 1);
        llama_sampler_accept(smpl, next_token);

        if (llama_vocab_is_eog(vocab, next_token)) {
            break;
        }

        char piece[256];
        const auto piece_len = llama_token_to_piece(vocab, next_token, piece, sizeof(piece), 0, true);
        response.append(piece, static_cast<std::size_t>(piece_len));

        nova::topic_log::debug(LogTopic, "Generated token {}: \"{}\"", i, std::string_view(piece, static_cast<std::size_t>(piece_len)));

        tokens = { next_token };
        batch = llama_batch_get_one(tokens.data(), static_cast<int>(tokens.size()));
    }

    nova::topic_log::info(LogTopic, "{}", response);
}

/**
 * @brief   Main entry point for the AIA tool.
 */
auto entrypoint(auto args) -> int {
    nova::topic_log::create(LogTopic);
    nova::log::load_env_levels();

    llama_log_set(ggml_log_to_topic, nullptr);

    const auto options = parse_args(args);
    if (not options) {
        return EXIT_FAILURE;
    }

    if (options->help) {
        return EXIT_SUCCESS;
    }

    ggml_backend_load_all();

    llama_model_params model_params = llama_model_default_params();
    llama_model* model = llama_model_load_from_file(options->model_path.c_str(), model_params);
    if (model == nullptr) {
        nova::topic_log::error(LogTopic, "Failed to load model: {}", options->model_path);
        return EXIT_FAILURE;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 4096;
    llama_context* ctx = llama_init_from_model(model, ctx_params);
    if (ctx == nullptr) {
        nova::topic_log::error(LogTopic, "Failed to create inference context");
        llama_model_free(model);
        return EXIT_FAILURE;
    }

    llama_sampler* smpl = build_sampler(*options);

    generate(model, ctx, smpl, options->prompt, read_context(options->context_path), options->n_predict);

    llama_sampler_free(smpl);
    llama_free(ctx);
    llama_model_free(model);

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
