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

#include <cstdlib>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace po = boost::program_options;

const std::string LogTopic = "aia";

/**
 * @brief   CLI options for the AIA tool.
 */
struct aia_options {
    std::string model_path {};
    std::string prompt {};
    int         n_predict { 128 };
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
        ("n-predict,n", po::value<int>()->default_value(128), "Number of tokens to generate")
    ;

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(args_vec).options(desc).run(), vm);
        if (vm.contains("help")) {
            std::cout << "Usage: aia --model <model.gguf> --prompt \"<text>\" [--n-predict <n>]\n";
            std::cout << desc << "\n";
            return aia_options { .help = true };
        }
        po::notify(vm);
    } catch (const std::exception& e) {
        nova::topic_log::error(LogTopic, "Error parsing arguments: {}", e.what());
        return std::nullopt;
    }

    return aia_options {
        .model_path = vm["model"].as<std::string>(),
        .prompt     = vm["prompt"].as<std::string>(),
        .n_predict  = vm["n-predict"].as<int>()
    };
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
 * @brief   Greedily generate `n_predict` tokens continuing the given prompt and log the response.
 *
 * @param   model       Loaded model.
 * @param   ctx         Inference context.
 * @param   prompt      Prompt text.
 * @param   n_predict   Number of tokens to generate.
 */
void generate(llama_model* model, llama_context* ctx, const std::string& prompt, int n_predict) {
    const llama_vocab* vocab = llama_model_get_vocab(model);

    auto tokens = tokenize_prompt(vocab, prompt);

    llama_batch batch = llama_batch_get_one(tokens.data(), static_cast<int>(tokens.size()));

    std::string response;

    for (int i = 0; i < n_predict; ++i) {
        if (llama_decode(ctx, batch) != 0) {
            nova::topic_log::error(LogTopic, "llama_decode failed");
            return;
        }

        auto* logits = llama_get_logits_ith(ctx, batch.n_tokens - 1);
        const auto n_vocab = llama_vocab_n_tokens(vocab);

        // Greedy sampling: pick the highest-probability token, no top-k/top-p/temperature.
        llama_token best_token = 0;
        float best_logit = logits[0];
        for (llama_token id = 1; id < n_vocab; ++id) {
            if (logits[id] > best_logit) {
                best_logit = logits[id];
                best_token = id;
            }
        }

        if (llama_vocab_is_eog(vocab, best_token)) {
            break;
        }

        char piece[256];
        const auto piece_len = llama_token_to_piece(vocab, best_token, piece, sizeof(piece), 0, true);
        response.append(piece, static_cast<std::size_t>(piece_len));

        nova::topic_log::debug(LogTopic, "Generated token {}: \"{}\"", i, std::string_view(piece, static_cast<std::size_t>(piece_len)));

        tokens = { best_token };
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
    ctx_params.n_ctx = 2048;
    llama_context* ctx = llama_init_from_model(model, ctx_params);
    if (ctx == nullptr) {
        nova::topic_log::error(LogTopic, "Failed to create inference context");
        llama_model_free(model);
        return EXIT_FAILURE;
    }

    generate(model, ctx, options->prompt, options->n_predict);

    llama_free(ctx);
    llama_model_free(model);

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
