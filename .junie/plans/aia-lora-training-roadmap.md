---
sessionId: session-260714-144156-1pry
---

# Roadmap

### Overview
This is a knowledge + tooling roadmap for going down the "train your own LoRA for AIA" rabbit hole, building on the existing `aia` CLI (chat loop, `--session`, `--memory`, and `--lora`/`--lora-scale` inference-time adapter loading already implemented). `llama.cpp`'s C API (used by `aia`) has no training/backward pass, so training always happens outside this repo (Python + Hugging Face `peft`/`transformers`/`bitsandbytes`), and the *only* piece that belongs inside `aia` is exporting your own conversation data into a trainable format.

### Stage 0 - Understand what a LoRA actually is (concept, no code)
- A LoRA (Low-Rank Adaptation) freezes the base model's weight matrices and injects small trainable low-rank matrices `A` (r x d) and `B` (d x r) next to selected linear layers (typically the attention `q_proj`/`v_proj`, sometimes `k_proj`/`o_proj`/MLP projections), so the effective weight becomes `W + BA * scale`.
- Only `A`/`B` (a few million parameters, `r` typically 8-64) get gradients; the base model (e.g. your Llama-3.2-1B/3B or Qwen2.5) stays frozen -> tiny, fast, reversible fine-tune vs. full fine-tuning.
- QLoRA = same idea, but the frozen base is loaded in 4-bit (NF4) so you can fine-tune bigger base models (e.g. up to 7-13B) on a single 16GB GPU like your 5080.
- Mental model to verify: after training, `adapter_model.safetensors` should contain only `A`/`B` matrices (megabytes, not gigabytes) - if it doesn't, something's wrong with your LoRA config (e.g. you accidentally fine-tuned everything).

### Stage 1 - Collect and export your own conversation data (in-repo tooling)
- Your `--session` files (single-conversation JSON transcripts) and `--memory` files (long-lived embedded exchange store) are already the raw material.
- Add an export step that converts one or more of these JSON files into a training-ready format: a JSONL file of `{"messages": [{"role": ..., "content": ...}, ...]}` records (the format Hugging Face's `trl`/`peft` SFT trainers expect), one record per (system + user + assistant) exchange or per full conversation.
- Filtering matters more than volume at this scale: drop exchanges where the assistant's answer was bad/wrong (you'll want a lightweight way to mark/skip turns), since a handful of curated examples on a small LoRA teaches style/behavior far more reliably than thousands of noisy ones.

### Stage 2 - Train the adapter externally (Hugging Face, outside this repo)
- Environment: a separate Python venv/conda env with `transformers`, `peft`, `trl`, `bitsandbytes`, `accelerate` - not part of the C++ build, kept in e.g. a `aia/training/` folder with its own `requirements.txt` and a `README` documenting the flow (not a Conan/CMake dependency).
- Load the *same base model* you use in `aia` (e.g. `meta-llama/Llama-3.2-1B-Instruct`) from Hugging Face in 4-bit via `bitsandbytes` (QLoRA), attach a `peft.LoraConfig` (start `r=16`, `alpha=32`, `target_modules=["q_proj","v_proj"]`, `dropout=0.05`), and run `trl`'s `SFTTrainer` over your exported JSONL for a handful of epochs (start with 1-3 epochs on a small dataset; watch training loss, don't overfit on a tiny personal dataset).
- Verification you can reason about as a human: loss should decrease smoothly and the model should visibly pick up specific phrasing/preferences from your data on a held-out prompt, without breaking general coherence - if it degenerates into repeating your training examples verbatim, the LoRA rank/epochs are too high for the dataset size.

### Stage 3 - Convert and apply the trained adapter back into AIA
- Convert the resulting Hugging Face adapter directory (`adapter_model.safetensors` + `adapter_config.json`) to GGUF using `llama.cpp`'s `convert_lora_to_gguf.py` (already referenced in `aia/README.md`'s existing LoRA section) against the same base GGUF model.
- Apply it via the CLI flags already implemented: `aia --model <base.gguf> --lora <adapter.gguf> --lora-scale 1.0 --prompt "..."`; use `--lora-scale` between 0 and 1 to dial in the effect strength without retraining.
- A/B it: run the same prompts with and without `--lora` (or `--lora-scale 0`) to confirm the adapter measurably changes behavior in the intended direction before trusting it day-to-day.

### Stage 4 - Make it periodic/repeatable
- Once the one-off flow works, the natural next step (documented as a roadmap item, not built yet) is a small script tying Stages 1-3 together (export -> train -> convert) so you can re-run it periodically as your `--session`/`--memory` files accumulate more real conversations, producing a refreshed adapter without ever touching the frozen base model.

### Scope
- In scope: an in-repo data-export tool (session/memory JSON -> training JSONL) and documentation of the full external training/conversion/apply loop, wired to the existing `--lora`/`--lora-scale` flags.
- Out of scope (explicitly not built by this plan): implementing the actual Python training script/pipeline in this repo, automating scheduled retraining, or any in-C++ training/backward-pass code (llama.cpp's C API doesn't support it).

# Delivery Steps

### ✓ Step 1: Add a session/memory data export tool to aia
aia can convert its own --session and --memory JSON files into a JSONL dataset ready for external LoRA/QLoRA fine-tuning.
- Add a new `--export-training-data <output.jsonl>` CLI option (or a small standalone script `aia/export-training-data` alongside `download-model.sh`) that reads one or more `--session`/`--memory` JSON files.
- Convert each conversation/exchange into a `{"messages": [...]}` JSONL record matching the format expected by Hugging Face `trl`'s SFT trainer (system/user/assistant roles preserved).
- Skip malformed or empty entries and log via `nova::topic_log` how many records were exported, at debug level, with the final count as the normal-level summary log.

### * Step 2: Document and scaffold the external training environment
A `aia/training/` folder documents and scaffolds the Python-side LoRA/QLoRA training flow, kept fully outside the CMake/Conan build.
- Add `aia/training/README.md` describing the recommended flow: load base model in 4-bit via `bitsandbytes`, attach a `peft.LoraConfig` (starting `r=16`, `alpha=32`, target `q_proj`/`v_proj`), train with `trl`'s `SFTTrainer` on the exported JSONL.
- Add `aia/training/requirements.txt` pinning `transformers`, `peft`, `trl`, `bitsandbytes`, `accelerate`.
- Add an example `aia/training/train_lora.py` script (parameterized by base model repo id and dataset path) as a concrete, runnable starting point rather than just prose.

###   Step 3: Document the conversion and apply/evaluate workflow
aia/README.md clearly documents the end-to-end loop from a freshly trained adapter to a testable GGUF adapter usable via the existing --lora flag.
- Extend the existing "LoRA fine-tuning (personalization)" section in `aia/README.md` with concrete commands: `convert_lora_to_gguf.py` invocation against the same base GGUF model, followed by `aia --model <base.gguf> --lora <adapter.gguf> --lora-scale <s> --prompt "..."`.
- Document an A/B verification step (same prompt with `--lora` vs `--lora-scale 0`) as the human-verifiable check that the adapter changed behavior as intended.
- Update the Roadmap section to reflect the new export tooling as done and note periodic/automated retraining as the next future step.