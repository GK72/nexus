# AIA

AIA (AI Assistant) is a subproject for experimenting with local LLM inference
and, eventually, a personalized assistant that learns from ongoing
discussions.

## Current scope

This is the initial scaffolding: a minimal CLI (`aia`) that loads a GGUF model
via [`llama.cpp`](https://github.com/ggml-org/llama.cpp) and greedily
completes a single prompt. No chat loop, sampling strategies, or memory/RAG
layer yet.

```bash
aia --model /path/to/model.gguf --prompt "Hello, "
```

The prompt is wrapped in a system message that instructs the model to ask
clarifying questions or request missing context instead of guessing, and is
formatted via the model's chat template. Attach extra context (e.g. code to
review) with `--context`:

```bash
aia --model /path/to/model.gguf --prompt "Review my code" --context ./main.cpp
```

The prompt (including any attached context) is fed to the model in chunks no
larger than `n_batch`, so long prompts (e.g. large files attached via
`--context`) don't hit `llama.cpp`'s `n_tokens_all <= cparams.n_batch`
assertion; it must still fit within the context window (`n_ctx`, currently
4096 tokens).

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

- Chat loop with proper sampling (temperature/top-k/top-p) instead of greedy decoding.
- Conversation persistence and a retrieval-augmented generation (RAG) layer over past discussions.
- Periodic LoRA/QLoRA fine-tuning on collected conversation data.
