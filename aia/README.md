# AIA

AIA (AI Assistant) is a subproject for experimenting with local LLM inference
and, eventually, a personalized assistant that learns from ongoing
discussions.

## Current scope

This is a CLI (`aia`) that loads a GGUF model via
[`llama.cpp`](https://github.com/ggml-org/llama.cpp) and runs an interactive,
multi-turn chat session using top-k/top-p/temperature sampling. No memory/RAG
layer yet.

```bash
aia --model /path/to/model.gguf --prompt "Hello, "
```

The session continues after the first response: you're prompted (`> `) for
follow-up messages on stdin, and the conversation history (including the
model's own prior replies) is kept for every subsequent turn. `--prompt` may
be omitted entirely to start straight in interactive mode. End the session
with `exit`, `quit`, or EOF (Ctrl-D):

```bash
aia --model /path/to/model.gguf   # first message is read interactively too
```

By default, the conversation is lost once the process exits. Attach `--session`
to a path and it's saved as JSON after every turn, then reloaded on the next
run so the same conversation (system prompt + full history) picks up where it
left off:

```bash
aia --model /path/to/model.gguf --session ./my-session.json --prompt "Hi"
# ... exit, then later:
aia --model /path/to/model.gguf --session ./my-session.json --prompt "Continue where we left off"
```

On resume, the whole persisted history is fed back through the model (there's
no KV cache to reuse across process runs, only within a single run), so
reloading a long session costs one full re-processing pass over that history.

Each turn is wrapped in a system message (sent once, at the start of the
conversation) that instructs the model to ask clarifying questions or request
missing context instead of guessing, and the full history is formatted via
the model's chat template on every turn. Only the newest turn is actually fed
through `llama_decode`, though -- the KV cache from earlier turns is reused,
so longer conversations don't get progressively slower to process. Attach
extra context (e.g. code to review) to the first message with `--context`:

```bash
aia --model /path/to/model.gguf --prompt "Review my code" --context ./main.cpp
```

The prompt (including any attached context) is fed to the model in chunks no
larger than `n_ubatch`, so long prompts (e.g. large files attached via
`--context`) don't hit `llama.cpp`'s per-decode batch limit; it must still fit
within the context window (`--ctx-size`, default 4096 tokens). If the whole
conversation (system prompt + history + any `--context` file) doesn't fit,
`aia` reports it clearly ("Prompt is too long for the context window")
instead of a raw decode failure -- raise `--ctx-size` or shorten the input:

```bash
aia --model /path/to/model.gguf --prompt "Review my code" --context ./big_file.cpp --ctx-size 8192
```

Each next token is drawn from a sampler chain (top-k -> top-p -> temperature
-> random draw) instead of always picking the single most likely token, so
responses vary between runs. Tune it via `--temperature`, `--top-k`, `--top-p`,
and `--seed` (fixing `--seed` makes output reproducible):

```bash
aia --model /path/to/model.gguf --prompt "Hello, " --temperature 0.7 --top-k 40 --top-p 0.9 --seed 42
```

Generation stops on an end-of-generation token, after `--n-predict` tokens
(default 1024), or when the context window (`--ctx-size`, default 4096
tokens) is exhausted, whichever comes first. If a response looks cut off, raise
`--n-predict` (enable debug logging, e.g. `SPDLOG_LEVEL=debug,aia=debug`, to
see which of these stopped it):

```bash
aia --model /path/to/model.gguf --prompt "Explain X in detail" --n-predict 2048
```

## Long-lived memory (retrieval-augmented recall)

Attach `--memory` to a JSON path to give `aia` a simple retrieval-augmented
memory that persists *across* sessions (independent of `--session`, which only
covers a single conversation's transcript):

```bash
aia --model /path/to/model.gguf --memory ./my-memory.json --prompt "My cat is named Whiskers"
# ... later, possibly in a different session:
aia --model /path/to/model.gguf --memory ./my-memory.json --prompt "What's my cat's name?"
```

Each turn's exchange (user message + response) is embedded with a dedicated
`llama.cpp` embeddings context (mean-pooled, cosine similarity) and appended to
the memory file. Before each new turn, the user's message is embedded the same
way and the `--memory-top-k` (default 3) most similar past exchanges above a
similarity threshold are recalled and injected as a system message ahead of
that turn, so the model can use them for context -- without ever growing the
chat history by more than what's actually relevant. Unlike `--session`, the
memory file is meant to accumulate across many separate conversations.

## LoRA fine-tuning (personalization)

`aia` cannot train a LoRA adapter itself -- `llama.cpp`'s C API only *loads
and applies* one at inference time, it has no training/backward pass. The
practical flow for "periodic fine-tuning on collected conversation data" is:

1. Collect conversation data (e.g. from `--session`/`--memory` files) as
   (prompt, response) pairs.
2. Fine-tune a LoRA/QLoRA adapter on top of the same base model using
   Hugging Face `peft`/`transformers` (outside this repo).
3. Convert the resulting adapter to GGUF with `llama.cpp`'s
   `convert_lora_to_gguf.py`.
4. Point `aia` at it with `--lora`:

```bash
aia --model /path/to/model.gguf --lora ./my-adapter.gguf --lora-scale 1.0 --prompt "Hi"
```

The adapter is applied on top of the frozen base model's weights for the
whole session (never modifying the base model file itself), so it's cheap to
swap or disable (drop `--lora`, or set `--lora-scale 0`) between runs.

## Exporting training data for LoRA fine-tuning

`aia` can convert its own `--session`/`--memory` JSON files into a JSONL
dataset in the `{"messages": [...]}` format expected by Hugging Face `trl`'s
`SFTTrainer` (see `aia/training/README.md`), so you can turn your own
conversations into training data for the LoRA fine-tuning flow described
above:

```bash
aia --export-training-data ./train.jsonl --export-source ./my-session.json --export-source ./my-memory.json
```

`--export-source` may be repeated to combine multiple `--session` and/or
`--memory` files into a single dataset; `--model` is not required for this
mode. Each `--session` file becomes one record per conversation (preserving
system/user/assistant roles), and each `--memory` file is split back into one
record per exchange. Malformed or empty entries are skipped (logged at debug
level); the final exported record count is logged normally.

## Downloading a model

Use `download-model.sh` to fetch a GGUF model from Hugging Face into `aia/models/`:

```bash
./download-model.sh --list          # show recommended presets
./download-model.sh qwen2.5-3b       # download a recommended preset
./download-model.sh someuser/some-model-GGUF some-model.Q4_K_M.gguf  # arbitrary repo/file
```

## Dependencies

`llama.cpp` is pulled in via Conan (`llama-cpp/b6565`), with `with_cuda=True`
set in `conanfile.txt` to enable CUDA acceleration. `nlohmann_json` (already a
repo-wide Conan dependency) is used to serialize `--session` files.

## Roadmap

- Tooling to help prepare `--session`/`--memory` data for external LoRA/QLoRA fine-tuning.
