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
larger than `n_batch`, so long prompts (e.g. large files attached via
`--context`) don't hit `llama.cpp`'s `n_tokens_all <= cparams.n_batch`
assertion; it must still fit within the context window (`n_ctx`, currently
4096 tokens).

Each next token is drawn from a sampler chain (top-k -> top-p -> temperature
-> random draw) instead of always picking the single most likely token, so
responses vary between runs. Tune it via `--temperature`, `--top-k`, `--top-p`,
and `--seed` (fixing `--seed` makes output reproducible):

```bash
aia --model /path/to/model.gguf --prompt "Hello, " --temperature 0.7 --top-k 40 --top-p 0.9 --seed 42
```

Generation stops on an end-of-generation token, after `--n-predict` tokens
(default 1024), or when the context window (`n_ctx`, 4096 tokens) is
exhausted, whichever comes first. If a response looks cut off, raise
`--n-predict` (enable debug logging, e.g. `SPDLOG_LEVEL=debug,aia=debug`, to
see which of these stopped it):

```bash
aia --model /path/to/model.gguf --prompt "Explain X in detail" --n-predict 2048
```

## Downloading a model

Use `download-model.sh` to fetch a GGUF model from Hugging Face into `aia/models/`:

```bash
./download-model.sh --list          # show recommended presets
./download-model.sh qwen2.5-3b       # download a recommended preset
./download-model.sh someuser/some-model-GGUF some-model.Q4_K_M.gguf  # arbitrary repo/file
```

## Dependencies

`llama.cpp` is pulled in via Conan (`llama-cpp/b6565`), with `with_cuda=True`
set in `conanfile.txt` to enable CUDA acceleration.

## Roadmap

- Conversation persistence and a retrieval-augmented generation (RAG) layer over past discussions.
- Periodic LoRA/QLoRA fine-tuning on collected conversation data.
