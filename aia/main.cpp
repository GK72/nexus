/**
 * Part of AIA (AI Assistant) Subproject.
 *
 * `llama.cpp`-based CLI for loading a GGUF model and running an interactive,
 * multi-turn chat session with top-k/top-p/temperature sampling. Conversation
 * history is retained for the whole session and the KV cache is reused
 * incrementally across turns; optionally, history is persisted to a JSON
 * session file so a conversation can be resumed across runs. A separate,
 * long-lived memory store (embeddings over past exchanges) can also be
 * attached so relevant snippets from earlier sessions are recalled via
 * simple retrieval-augmented generation (RAG). A LoRA adapter (trained
 * externally, e.g. via HF PEFT/QLoRA on collected conversations) can be
 * applied at inference time for cheap, reversible personalization; the
 * collected --session/--memory files can themselves be exported into a
 * JSONL dataset (--export-training-data) for that external training step.
 *
 * @author  Gábor Krisztián Girhiny and Junie
 * @date    2026-07-14
 */

#include <libnova/log.hpp>
#include <libnova/error.hpp>
#include <libnova/main.hpp>

#include <llama.h>

#include <boost/program_options.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
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
    std::string session_path {};
    std::string memory_path {};
    int         memory_top_k { 3 };
    std::string lora_path {};
    float       lora_scale { 1.0F };
    int         n_predict { 1024 };
    int         ctx_size { 4096 };
    float       temperature { 0.8F };
    int         top_k { 40 };
    float       top_p { 0.95F };
    uint32_t    seed { LLAMA_DEFAULT_SEED };
    std::vector<std::string> export_training_sources {};
    std::string export_training_output {};
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
        ("model,m", po::value<std::string>(), "Path to a GGUF model file (not required when using --export-training-data)")
        ("prompt,p", po::value<std::string>(), "First prompt; if omitted, it is read interactively. Either way, the session continues as a multi-turn chat until you type \"exit\" or \"quit\" (or send EOF)")
        ("context,c", po::value<std::string>(), "Path to a file whose contents are attached as context (e.g. code to review)")
        ("session,s", po::value<std::string>(), "Path to a JSON file used to persist and reload the conversation history across runs")
        ("memory,M", po::value<std::string>(), "Path to a JSON file used as a long-lived memory store (embeddings of past exchanges) for retrieval-augmented recall")
        ("memory-top-k", po::value<int>()->default_value(3), "Number of past exchanges to recall from --memory per turn")
        ("lora", po::value<std::string>(), "Path to a GGUF LoRA adapter to apply on top of the base model (trained externally, e.g. via HF PEFT/QLoRA and converted with llama.cpp's convert_lora_to_gguf.py)")
        ("lora-scale", po::value<float>()->default_value(1.0F), "Scale factor applied to the LoRA adapter (1.0 = as trained; 0.0 effectively disables it without removing --lora)")
        ("export-training-data", po::value<std::string>(), "Export one or more --session/--memory JSON files (given via --export-source) into a JSONL dataset ready for external LoRA/QLoRA fine-tuning, then exit")
        ("export-source", po::value<std::vector<std::string>>()->multitoken(), "Path to a --session or --memory JSON file to include in --export-training-data (may be repeated)")
        ("n-predict,n", po::value<int>()->default_value(1024), "Number of tokens to generate")
        ("ctx-size", po::value<int>()->default_value(4096), "Context window size in tokens; the whole conversation, including any --context file, must fit within it")
        ("temperature,t", po::value<float>()->default_value(0.8F), "Sampling temperature (higher = more random)")
        ("top-k", po::value<int>()->default_value(40), "Keep only the top K most likely tokens (0 = disabled)")
        ("top-p", po::value<float>()->default_value(0.95F), "Keep the smallest set of tokens whose cumulative probability exceeds P (1.0 = disabled)")
        ("seed", po::value<uint32_t>()->default_value(LLAMA_DEFAULT_SEED), "Random seed for sampling (default: random)")
    ;

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(args_vec).options(desc).run(), vm);
        if (vm.contains("help")) {
            std::cout << "Usage: aia --model <model.gguf> --prompt \"<text>\" [--context <file>] [--session <file>] [--memory <file>] [--memory-top-k <n>] [--lora <file>] [--lora-scale <s>] [--n-predict <n>] "
                         "[--ctx-size <n>] [--temperature <t>] [--top-k <k>] [--top-p <p>] [--seed <s>]\n"
                         "       aia --model <model.gguf> --export-training-data <output.jsonl> --export-source <session_or_memory.json> [--export-source <file2.json> ...]\n";
            std::cout << desc << "\n";
            return aia_options { .help = true };
        }
        po::notify(vm);
    } catch (const std::exception& e) {
        nova::topic_log::error(LogTopic, "Error parsing arguments: {}", e.what());
        return std::nullopt;
    }

    if (not vm.contains("model") and not vm.contains("export-training-data")) {
        nova::topic_log::error(LogTopic, "Missing required option: --model");
        return std::nullopt;
    }

    return aia_options {
        .model_path   = vm.contains("model") ? vm["model"].as<std::string>() : "",
        .prompt       = vm.contains("prompt") ? vm["prompt"].as<std::string>() : "",
        .context_path = vm.contains("context") ? vm["context"].as<std::string>() : "",
        .session_path = vm.contains("session") ? vm["session"].as<std::string>() : "",
        .memory_path  = vm.contains("memory") ? vm["memory"].as<std::string>() : "",
        .memory_top_k = vm["memory-top-k"].as<int>(),
        .lora_path    = vm.contains("lora") ? vm["lora"].as<std::string>() : "",
        .lora_scale   = vm["lora-scale"].as<float>(),
        .n_predict    = vm["n-predict"].as<int>(),
        .ctx_size     = vm["ctx-size"].as<int>(),
        .temperature  = vm["temperature"].as<float>(),
        .top_k        = vm["top-k"].as<int>(),
        .top_p        = vm["top-p"].as<float>(),
        .seed         = vm["seed"].as<uint32_t>(),
        .export_training_sources = vm.contains("export-source") ? vm["export-source"].as<std::vector<std::string>>() : std::vector<std::string>{},
        .export_training_output  = vm.contains("export-training-data") ? vm["export-training-data"].as<std::string>() : ""
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
 * @brief   Load a previously persisted conversation history from a JSON session file.
 *
 * The file is a JSON array of `{"role": ..., "content": ...}` objects, in order. Missing
 * or unreadable/malformed files are treated as "no history yet" rather than a hard error,
 * since a session file legitimately doesn't exist on the very first run.
 *
 * @param   session_path    Path to the session file, or empty to disable persistence.
 *
 * @return  Loaded history (possibly empty).
 */
[[nodiscard]] auto load_session(const std::string& session_path) -> std::vector<chat_message> {
    std::vector<chat_message> history;
    if (session_path.empty()) {
        return history;
    }

    std::ifstream file(session_path);
    if (not file) {
        nova::topic_log::debug(LogTopic, "No existing session file at {}, starting fresh", session_path);
        return history;
    }

    try {
        nlohmann::json json;
        file >> json;
        for (const auto& entry : json) {
            history.push_back({ entry.at("role").get<std::string>(), entry.at("content").get<std::string>() });
        }
        nova::topic_log::debug(LogTopic, "Loaded {} message(s) from session file {}", history.size(), session_path);
    } catch (const std::exception& e) {
        nova::topic_log::debug(LogTopic, "Failed to parse session file {}: {}; starting fresh", session_path, e.what());
        history.clear();
    }

    return history;
}

/**
 * @brief   Persist the current conversation history to a JSON session file.
 *
 * Overwrites the file with the full history (including the system prompt) so the next
 * run of `aia --session <same file>` can resume the conversation from where it left off.
 *
 * @param   session_path    Path to the session file, or empty to disable persistence.
 * @param   history         Full conversation history to persist.
 */
void save_session(const std::string& session_path, const std::vector<chat_message>& history) {
    if (session_path.empty()) {
        return;
    }

    nlohmann::json json = nlohmann::json::array();
    for (const auto& msg : history) {
        json.push_back({ { "role", msg.role }, { "content", msg.content } });
    }

    std::ofstream file(session_path);
    if (not file) {
        nova::topic_log::debug(LogTopic, "Could not open session file for writing: {}", session_path);
        return;
    }

    file << json.dump(2);
    nova::topic_log::debug(LogTopic, "Saved {} message(s) to session file {}", history.size(), session_path);
}

/**
 * @brief   Export one or more --session/--memory JSON files into a JSONL dataset ready
 *          for external Hugging Face `trl`/`peft` LoRA/QLoRA fine-tuning.
 *
 * Each `--session` file (a JSON array of `{"role", "content"}` messages) becomes a single
 * `{"messages": [...]}` JSONL record, preserving system/user/assistant roles as-is. Each
 * `--memory` file (a JSON array of `{"text", "embedding"}` entries, where `text` is a
 * "User: ...\nAssistant: ..." exchange) is split back into one `{"messages": [...]}`
 * record per exchange. Files that don't parse as either shape, or that yield no usable
 * messages, are skipped rather than aborting the whole export.
 *
 * @param   sources Paths to `--session`/`--memory` JSON files to include.
 * @param   output  Path to the JSONL file to write.
 *
 * @return  Number of records written.
 */
[[nodiscard]] auto export_training_data(const std::vector<std::string>& sources, const std::string& output) -> std::size_t {
    std::ofstream out(output);
    if (not out) {
        nova::topic_log::error(LogTopic, "Could not open output file for writing: {}", output);
        return 0;
    }

    std::size_t written = 0;
    std::size_t skipped = 0;

    for (const auto& source : sources) {
        std::ifstream file(source);
        if (not file) {
            nova::topic_log::debug(LogTopic, "Could not open source file: {}", source);
            ++skipped;
            continue;
        }

        nlohmann::json json;
        try {
            file >> json;
        } catch (const std::exception& e) {
            nova::topic_log::debug(LogTopic, "Failed to parse source file {}: {}", source, e.what());
            ++skipped;
            continue;
        }

        if (not json.is_array()) {
            nova::topic_log::debug(LogTopic, "Source file {} is not a JSON array, skipping", source);
            ++skipped;
            continue;
        }

        // Disambiguate a --session file (entries have "role"/"content") from a --memory
        // file (entries have "text"/"embedding") by inspecting the first entry's shape.
        const bool looks_like_session = not json.empty() and json.front().contains("role") and json.front().contains("content");

        if (looks_like_session) {
            nlohmann::json messages = nlohmann::json::array();
            for (const auto& entry : json) {
                if (not entry.contains("role") or not entry.contains("content")) {
                    continue;
                }
                messages.push_back({ { "role", entry.at("role") }, { "content", entry.at("content") } });
            }
            if (messages.empty()) {
                nova::topic_log::debug(LogTopic, "No usable messages in session file {}, skipping", source);
                ++skipped;
                continue;
            }
            out << nlohmann::json{ { "messages", messages } }.dump() << "\n";
            ++written;
            continue;
        }

        // Memory file: split each "User: ...\nAssistant: ..." exchange back into
        // separate user/assistant messages, one JSONL record per exchange.
        for (const auto& entry : json) {
            if (not entry.contains("text")) {
                ++skipped;
                continue;
            }
            const auto text = entry.at("text").get<std::string>();
            const auto user_prefix = std::string("User: ");
            const auto assistant_marker = std::string("\nAssistant: ");
            const auto assistant_pos = text.find(assistant_marker);
            if (text.rfind(user_prefix, 0) != 0 or assistant_pos == std::string::npos) {
                nova::topic_log::debug(LogTopic, "Malformed memory entry in {}, skipping", source);
                ++skipped;
                continue;
            }
            const auto user_content = text.substr(user_prefix.size(), assistant_pos - user_prefix.size());
            const auto assistant_content = text.substr(assistant_pos + assistant_marker.size());
            if (user_content.empty() or assistant_content.empty()) {
                ++skipped;
                continue;
            }
            nlohmann::json messages = nlohmann::json::array();
            messages.push_back({ { "role", "user" }, { "content", user_content } });
            messages.push_back({ { "role", "assistant" }, { "content", assistant_content } });
            out << nlohmann::json{ { "messages", messages } }.dump() << "\n";
            ++written;
        }
    }

    nova::topic_log::debug(LogTopic, "Skipped {} malformed/empty entrie(s) while exporting training data", skipped);
    nova::topic_log::info(LogTopic, "Exported {} record(s) to {}", written, output);

    return written;
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
 * @brief   A past exchange stored in the long-lived memory store, together with the
 *          embedding used to find it again by similarity.
 */
struct memory_entry {
    std::string text;
    std::vector<float> embedding;
};

/**
 * @brief   Load the long-lived memory store from a JSON file.
 *
 * Same tolerant-of-absence/malformed behavior as `load_session`: a missing or broken
 * store just means "no memories yet", not a hard error.
 *
 * @param   memory_path Path to the memory file, or empty to disable memory.
 *
 * @return  Loaded entries (possibly empty).
 */
[[nodiscard]] auto load_memories(const std::string& memory_path) -> std::vector<memory_entry> {
    std::vector<memory_entry> memories;
    if (memory_path.empty()) {
        return memories;
    }

    std::ifstream file(memory_path);
    if (not file) {
        nova::topic_log::debug(LogTopic, "No existing memory file at {}, starting with empty memory", memory_path);
        return memories;
    }

    try {
        nlohmann::json json;
        file >> json;
        for (const auto& entry : json) {
            memories.push_back({
                entry.at("text").get<std::string>(),
                entry.at("embedding").get<std::vector<float>>()
            });
        }
        nova::topic_log::debug(LogTopic, "Loaded {} memory entrie(s) from {}", memories.size(), memory_path);
    } catch (const std::exception& e) {
        nova::topic_log::debug(LogTopic, "Failed to parse memory file {}: {}; starting with empty memory", memory_path, e.what());
        memories.clear();
    }

    return memories;
}

/**
 * @brief   Persist the long-lived memory store to a JSON file.
 *
 * @param   memory_path Path to the memory file, or empty to disable memory.
 * @param   memories    Full set of memory entries to persist.
 */
void save_memories(const std::string& memory_path, const std::vector<memory_entry>& memories) {
    if (memory_path.empty()) {
        return;
    }

    nlohmann::json json = nlohmann::json::array();
    for (const auto& entry : memories) {
        json.push_back({ { "text", entry.text }, { "embedding", entry.embedding } });
    }

    std::ofstream file(memory_path);
    if (not file) {
        nova::topic_log::debug(LogTopic, "Could not open memory file for writing: {}", memory_path);
        return;
    }

    file << json.dump();
    nova::topic_log::debug(LogTopic, "Saved {} memory entrie(s) to {}", memories.size(), memory_path);
}

/**
 * @brief   Compute a mean-pooled embedding for a piece of text using a dedicated
 *          embeddings-enabled context over the same model used for generation.
 *
 * Uses a separate `llama_context` (rather than the chat context) because embeddings
 * require `LLAMA_POOLING_TYPE_MEAN` and `llama_set_embeddings(true)`, which would
 * otherwise interfere with normal causal-LM decoding for the chat loop. The memory
 * is cleared before and after so this never affects, or is affected by, the chat
 * context's KV cache (they are entirely separate contexts/memories anyway).
 *
 * @param   embed_ctx   Dedicated embeddings context (embeddings=true, pooling=MEAN).
 * @param   vocab       Model vocabulary.
 * @param   text        Text to embed.
 *
 * @return  Normalized embedding vector, or an empty vector on failure.
 */
[[nodiscard]] auto embed_text(llama_context* embed_ctx, const llama_vocab* vocab, const std::string& text) -> std::vector<float> {
    auto tokens = tokenize_prompt(vocab, text);
    const auto n_ctx = static_cast<std::size_t>(llama_n_ctx(embed_ctx));
    if (tokens.size() > n_ctx) {
        tokens.resize(n_ctx);
    }

    llama_memory_clear(llama_get_memory(embed_ctx), true);

    llama_batch batch = llama_batch_get_one(tokens.data(), static_cast<int>(tokens.size()));
    if (llama_decode(embed_ctx, batch) != 0) {
        nova::topic_log::debug(LogTopic, "Failed to compute embedding for text");
        return {};
    }

    const float* raw = llama_get_embeddings_seq(embed_ctx, 0);
    if (raw == nullptr) {
        nova::topic_log::debug(LogTopic, "No pooled embedding available for text");
        return {};
    }

    const auto n_embd = static_cast<std::size_t>(llama_model_n_embd(llama_get_model(embed_ctx)));
    std::vector<float> embedding(raw, raw + n_embd);

    float norm = 0.0F;
    for (const float v : embedding) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 0.0F) {
        for (float& v : embedding) {
            v /= norm;
        }
    }

    return embedding;
}

/**
 * @brief   Cosine similarity between two (assumed normalized) embeddings.
 */
[[nodiscard]] auto cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) -> float {
    if (a.size() != b.size() or a.empty()) {
        return 0.0F;
    }

    float dot = 0.0F;
    for (std::size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
    }
    return dot;
}

/**
 * @brief   Find the memory entries most relevant to a query embedding.
 *
 * @param   memories        Memory store to search.
 * @param   query_embedding Embedding of the current user message.
 * @param   top_k           Maximum number of entries to return.
 * @param   min_similarity  Minimum cosine similarity for an entry to be considered relevant.
 *
 * @return  Up to `top_k` entry texts, most relevant first.
 */
[[nodiscard]] auto retrieve_relevant_memories(
        const std::vector<memory_entry>& memories,
        const std::vector<float>& query_embedding,
        int top_k,
        float min_similarity = 0.5F)
        -> std::vector<std::string>
{
    std::vector<std::pair<float, std::size_t>> scored;
    scored.reserve(memories.size());
    for (std::size_t i = 0; i < memories.size(); ++i) {
        const auto sim = cosine_similarity(query_embedding, memories[i].embedding);
        if (sim >= min_similarity) {
            scored.emplace_back(sim, i);
        }
    }

    std::ranges::sort(scored, std::greater<>{}, &std::pair<float, std::size_t>::first);

    std::vector<std::string> result;
    for (std::size_t i = 0; i < scored.size() and static_cast<int>(i) < top_k; ++i) {
        result.push_back(memories[scored[i].second].text);
    }
    return result;
}

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
    // `llama_decode` requires each individual call's batch to fit within `n_ubatch`
    // (the actual per-decode processing unit), not just `n_batch`: a chunk larger
    // than `n_ubatch` fails with "failed to find a memory slot" even though it's
    // still within `n_batch`.
    const auto n_ubatch = llama_n_ubatch(ctx);
    std::size_t n_fed = 0;
    llama_batch batch {};
    while (n_fed < tokens.size()) {
        const auto chunk_size = std::min(static_cast<std::size_t>(n_ubatch), tokens.size() - n_fed);
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

    // Check this up front rather than letting `llama_decode` fail mid-prompt with an
    // opaque "failed to find a memory slot" once the KV cache fills up: a prompt (plus
    // any attached --context file) that doesn't fit in the context window can never be
    // fully processed, so fail clearly and point at the fix (--ctx-size).
    const auto n_ctx = llama_n_ctx(ctx);
    if (full_tokens.size() >= n_ctx) {
        nova::topic_log::error(
            LogTopic,
            "Prompt is too long for the context window ({} tokens, but n_ctx={}); "
            "shorten it (e.g. a smaller --context file) or raise --ctx-size",
            full_tokens.size(), n_ctx
        );
        return {};
    }

    auto batch = feed_tokens(ctx, new_tokens);
    if (not batch) {
        return {};
    }
    n_past = full_tokens.size();

    std::string response;

    // Track how many tokens have been fed into the KV cache so we can stop before
    // exceeding the context window, instead of letting `llama_decode` fail and
    // silently cutting the response off mid-token.
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
 * @param   session_path    Path to a JSON file used to reload and persist the conversation
 *                          history across runs, or empty to disable persistence.
 * @param   embed_ctx       Dedicated embeddings context for memory recall, or `nullptr` to
 *                          disable retrieval-augmented recall entirely.
 * @param   memory_path     Path to the long-lived memory store, or empty to disable it.
 * @param   memory_top_k    Maximum number of recalled past exchanges to inject per turn.
 */
void chat_loop(
        llama_model* model, llama_context* ctx, llama_sampler* smpl,
        const std::string& first_prompt, const std::optional<std::string>& context, int n_predict,
        const std::string& session_path,
        llama_context* embed_ctx, const std::string& memory_path, int memory_top_k)
{
    const llama_vocab* vocab = llama_model_get_vocab(model);

    auto history = load_session(session_path);
    if (history.empty()) {
        history.push_back({ "system", SystemPrompt });
    }
    auto memories = load_memories(memory_path);

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

        // Recall relevant past exchanges (from this or earlier sessions) before the new
        // turn is added, so the retrieval query is the user's own words, not anything
        // already injected. This is inserted as a plain history entry (rather than kept
        // separate) so it stays consistent with the KV cache like every other turn.
        if (embed_ctx != nullptr and not memory_path.empty()) {
            const auto query_embedding = embed_text(embed_ctx, vocab, prompt);
            const auto recalled = retrieve_relevant_memories(memories, query_embedding, memory_top_k);
            if (not recalled.empty()) {
                std::string recall_block = "Relevant excerpts from earlier discussions (for context, may or may not be relevant):\n";
                for (const auto& snippet : recalled) {
                    recall_block += "---\n" + snippet + "\n";
                }
                nova::topic_log::debug(LogTopic, "Recalled {} memory entrie(s) for this turn", recalled.size());
                history.push_back({ "system", recall_block });
            }
        }

        const auto user_message = first_turn ? build_user_message(prompt, context) : prompt;
        history.push_back({ "user", user_message });

        const auto response = generate(model, ctx, smpl, history, n_past, n_predict);
        history.push_back({ "assistant", response });

        save_session(session_path, history);

        if (embed_ctx != nullptr and not memory_path.empty()) {
            const auto exchange_text = "User: " + user_message + "\nAssistant: " + response;
            auto exchange_embedding = embed_text(embed_ctx, vocab, exchange_text);
            if (not exchange_embedding.empty()) {
                memories.push_back({ exchange_text, std::move(exchange_embedding) });
                save_memories(memory_path, memories);
            }
        }

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

    if (not options->export_training_output.empty()) {
        if (options->export_training_sources.empty()) {
            nova::topic_log::error(LogTopic, "--export-training-data requires at least one --export-source");
            return EXIT_FAILURE;
        }
        const auto record_count = export_training_data(options->export_training_sources, options->export_training_output);
        return record_count > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (options->model_path.empty()) {
        nova::topic_log::error(LogTopic, "Missing required option: --model");
        return EXIT_FAILURE;
    }

    ggml_backend_load_all();

    llama_model_params model_params = llama_model_default_params();
    llama_model* model = llama_model_load_from_file(options->model_path.c_str(), model_params);
    if (model == nullptr) {
        nova::topic_log::error(LogTopic, "Failed to load model: {}", options->model_path);
        return EXIT_FAILURE;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = static_cast<uint32_t>(options->ctx_size);
    llama_context* ctx = llama_init_from_model(model, ctx_params);
    if (ctx == nullptr) {
        nova::topic_log::error(LogTopic, "Failed to create inference context");
        llama_model_free(model);
        return EXIT_FAILURE;
    }

    llama_sampler* smpl = build_sampler(*options);

    // A LoRA adapter is loaded and trained externally (e.g. HF PEFT/QLoRA, then
    // converted to GGUF via llama.cpp's convert_lora_to_gguf.py); aia only applies
    // it at inference time, on top of the frozen base model's weights, which is
    // exactly the "cheap, reversible personalization" approach discussed for
    // periodic fine-tuning on collected conversation data.
    llama_adapter_lora* lora = nullptr;
    if (not options->lora_path.empty()) {
        lora = llama_adapter_lora_init(model, options->lora_path.c_str());
        if (lora == nullptr) {
            nova::topic_log::error(LogTopic, "Failed to load LoRA adapter: {}", options->lora_path);
            llama_sampler_free(smpl);
            llama_free(ctx);
            llama_model_free(model);
            return EXIT_FAILURE;
        }
        if (llama_set_adapter_lora(ctx, lora, options->lora_scale) != 0) {
            nova::topic_log::error(LogTopic, "Failed to apply LoRA adapter: {}", options->lora_path);
        } else {
            nova::topic_log::debug(LogTopic, "Applied LoRA adapter {} (scale={})", options->lora_path, options->lora_scale);
        }
    }

    // A dedicated embeddings context is only needed for memory recall; keep it entirely
    // separate from the chat context so pooled-embedding decoding never disturbs the
    // chat KV cache (or vice versa).
    llama_context* embed_ctx = nullptr;
    if (not options->memory_path.empty()) {
        llama_context_params embed_params = llama_context_default_params();
        embed_params.n_ctx = static_cast<uint32_t>(options->ctx_size);
        embed_params.embeddings = true;
        embed_params.pooling_type = LLAMA_POOLING_TYPE_MEAN;
        embed_ctx = llama_init_from_model(model, embed_params);
        if (embed_ctx == nullptr) {
            nova::topic_log::debug(LogTopic, "Failed to create embeddings context; memory recall disabled for this run");
        }
    }

    chat_loop(
        model, ctx, smpl, options->prompt, read_context(options->context_path), options->n_predict, options->session_path,
        embed_ctx, options->memory_path, options->memory_top_k
    );

    if (embed_ctx != nullptr) {
        llama_free(embed_ctx);
    }
    if (lora != nullptr) {
        llama_adapter_lora_free(lora);
    }
    llama_sampler_free(smpl);
    llama_free(ctx);
    llama_model_free(model);

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
