/**
 * Part of AIA (AI Assistant) Subproject.
 *
 * `llama.cpp`-based CLI for loading a GGUF model and running an interactive,
 * multi-turn chat session with top-k/top-p/temperature sampling. Conversation
 * history is retained for the whole session and the KV cache is reused
 * incrementally across turns. No cross-session persistence or RAG layer yet.
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
    int         n_predict { 1024 };
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
        ("prompt,p", po::value<std::string>(), "First prompt; if omitted, it is read interactively. Either way, the session continues as a multi-turn chat until you type \"exit\" or \"quit\" (or send EOF)")
        ("context,c", po::value<std::string>(), "Path to a file whose contents are attached as context (e.g. code to review)")
        ("n-predict,n", po::value<int>()->default_value(1024), "Number of tokens to generate")
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
        .prompt       = vm.contains("prompt") ? vm["prompt"].as<std::string>() : "",
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
 * @brief   A single message in a conversation (role + content), owning its strings so
 *          pointers handed to `llama_chat_message` stay valid for the call's duration.
 */
struct chat_message {
    std::string role;
    std::string content;
};

/**
 * @brief   Apply the model's chat template to a full conversation history.
 *
 * Falls back to concatenating the last message's content if the model has no chat template.
 *
 * @param   model       Loaded model.
 * @param   history     Full conversation so far (system + alternating user/assistant turns).
 *
 * @return  Formatted prompt ready for tokenization, including the assistant generation prompt.
 */
[[nodiscard]] auto apply_chat_template(llama_model* model, const std::vector<chat_message>& history) -> std::string {
    const char* tmpl = llama_model_chat_template(model, nullptr);
    if (tmpl == nullptr) {
        nova::topic_log::debug(LogTopic, "Model has no chat template, falling back to raw prompt");
        return history.empty() ? std::string{} : history.back().content;
    }

    std::vector<llama_chat_message> messages;
    messages.reserve(history.size());
    std::size_t total_size = 0;
    for (const auto& msg : history) {
        messages.push_back({ msg.role.c_str(), msg.content.c_str() });
        total_size += msg.content.size();
    }

    std::vector<char> buf(2 * total_size + 256);
    auto n_written = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, buf.data(), static_cast<int32_t>(buf.size()));
    if (n_written < 0) {
        nova::topic_log::debug(LogTopic, "Failed to apply chat template, falling back to raw prompt");
        return history.empty() ? std::string{} : history.back().content;
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
 * @brief   Feed a sequence of tokens (already appended past `n_used`) into the KV cache in
 *          chunks no larger than `n_batch`, since `llama_decode` requires the batch to fit.
 *
 * @param   ctx     Inference context.
 * @param   tokens  Tokens to feed.
 *
 * @return  The batch used for the last chunk (its logits are ready for sampling), or
 *          `std::nullopt` if decoding failed.
 */
[[nodiscard]] auto feed_tokens(llama_context* ctx, const std::vector<llama_token>& tokens) -> std::optional<llama_batch> {
    const auto n_batch = llama_n_batch(ctx);
    std::size_t n_fed = 0;
    llama_batch batch {};
    while (n_fed < tokens.size()) {
        const auto chunk_size = std::min(static_cast<std::size_t>(n_batch), tokens.size() - n_fed);
        batch = llama_batch_get_one(const_cast<llama_token*>(tokens.data()) + n_fed, static_cast<int>(chunk_size));
        if (llama_decode(ctx, batch) != 0) {
            nova::topic_log::error(LogTopic, "llama_decode failed while processing prompt");
            return std::nullopt;
        }
        n_fed += chunk_size;
    }
    return batch;
}

/**
 * @brief   Generate a response continuing `history` (whose last message is the new user
 *          turn), reusing the KV cache across calls, and log it.
 *
 * Only the tokens beyond the previously processed prefix (`n_past`) are fed to the model,
 * since the KV cache already holds the earlier turns; this keeps multi-turn conversations
 * cheap instead of reprocessing the whole history every turn. `n_past` is updated in place
 * to reflect everything now in the KV cache (prompt + generated response).
 *
 * @param   model       Loaded model.
 * @param   ctx         Inference context.
 * @param   smpl        Sampler chain used to pick each next token.
 * @param   history     Full conversation so far, including the latest user message.
 * @param   n_past      In/out: number of tokens already in the KV cache; advanced by this call.
 * @param   n_predict   Maximum number of tokens to generate for this turn.
 *
 * @return  The generated response text (empty on decode failure).
 */
[[nodiscard]] auto generate(llama_model* model, llama_context* ctx, llama_sampler* smpl, const std::vector<chat_message>& history, std::size_t& n_past, int n_predict) -> std::string {
    const llama_vocab* vocab = llama_model_get_vocab(model);

    const auto formatted_prompt = apply_chat_template(model, history);
    nova::topic_log::debug(LogTopic, "Formatted prompt: {}", formatted_prompt);

    auto full_tokens = tokenize_prompt(vocab, formatted_prompt);
    if (full_tokens.size() < n_past) {
        // Should not happen (history only grows), but guard against a template that
        // doesn't produce a stable prefix.
        nova::topic_log::debug(LogTopic, "Formatted prompt shrank versus KV cache, resetting n_past");
        n_past = 0;
    }

    const std::vector<llama_token> new_tokens(full_tokens.begin() + static_cast<std::ptrdiff_t>(n_past), full_tokens.end());

    auto batch = feed_tokens(ctx, new_tokens);
    if (not batch) {
        return {};
    }
    n_past = full_tokens.size();

    std::string response;

    // Track how many tokens have been fed into the KV cache so we can stop before
    // exceeding the context window, instead of letting `llama_decode` fail and
    // silently cutting the response off mid-token.
    const auto n_ctx = llama_n_ctx(ctx);
    bool reached_eog = false;
    int generated = 0;
    std::vector<llama_token> next_tokens;

    for (int i = 0; i < n_predict; ++i) {
        if (n_past >= n_ctx) {
            nova::topic_log::debug(LogTopic, "Stopping generation: context window ({} tokens) exhausted", n_ctx);
            break;
        }

        if (i > 0) {
            const auto rebatched = feed_tokens(ctx, next_tokens);
            if (not rebatched) {
                break;
            }
            batch = rebatched;
            n_past += next_tokens.size();
        }

        const llama_token next_token = llama_sampler_sample(smpl, ctx, batch->n_tokens - 1);
        llama_sampler_accept(smpl, next_token);

        if (llama_vocab_is_eog(vocab, next_token)) {
            reached_eog = true;
            break;
        }

        char piece[256];
        const auto piece_len = llama_token_to_piece(vocab, next_token, piece, sizeof(piece), 0, true);
        response.append(piece, static_cast<std::size_t>(piece_len));
        ++generated;

        nova::topic_log::debug(LogTopic, "Generated token {}: \"{}\"", i, std::string_view(piece, static_cast<std::size_t>(piece_len)));

        next_tokens = { next_token };
    }

    if (not reached_eog) {
        nova::topic_log::debug(
            LogTopic,
            "Response truncated after {} tokens without reaching an end-of-generation token "
            "(hit n_predict={} or the context window); increase --n-predict if the answer looks cut off",
            generated, n_predict
        );
    }

    nova::topic_log::info(LogTopic, "{}", response);

    return response;
}

/**
 * @brief   Run an interactive, multi-turn chat session.
 *
 * The conversation history (system prompt plus every user/assistant turn) is retained
 * for the whole session and re-fed through the model's chat template each turn, but the
 * KV cache is reused incrementally (`n_past`) so only the newest turn is actually decoded.
 * The first turn's user message optionally carries attached context (e.g. a file to
 * review); later turns are plain follow-up messages. The session ends on "exit"/"quit" or EOF.
 *
 * @param   model           Loaded model.
 * @param   ctx             Inference context.
 * @param   smpl            Sampler chain used to pick each next token.
 * @param   first_prompt    First user message; if empty, it is read from stdin like every later turn.
 * @param   context         Optional context to attach to the first user message.
 * @param   n_predict       Maximum number of tokens to generate per turn.
 */
void chat_loop(llama_model* model, llama_context* ctx, llama_sampler* smpl, const std::string& first_prompt, const std::optional<std::string>& context, int n_predict) {
    std::vector<chat_message> history { { "system", SystemPrompt } };
    std::size_t n_past = 0;
    bool first_turn = true;

    while (true) {
        std::string prompt = first_prompt;
        if (not first_turn or prompt.empty()) {
            std::cout << "> ";
            if (not std::getline(std::cin, prompt)) {
                break;
            }
        }

        if (prompt == "exit" or prompt == "quit") {
            break;
        }

        if (prompt.empty()) {
            continue;
        }

        const auto user_message = first_turn ? build_user_message(prompt, context) : prompt;
        history.push_back({ "user", user_message });

        const auto response = generate(model, ctx, smpl, history, n_past, n_predict);
        history.push_back({ "assistant", response });

        first_turn = false;
    }
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

    chat_loop(model, ctx, smpl, options->prompt, read_context(options->context_path), options->n_predict);

    llama_sampler_free(smpl);
    llama_free(ctx);
    llama_model_free(model);

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
