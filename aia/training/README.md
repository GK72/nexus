# AIA training (external, Python)

This folder holds the Python-side LoRA/QLoRA training flow for AIA. It is
deliberately kept **outside** the CMake/Conan build: `llama.cpp`'s C API
(used by `aia`) has no training/backward pass, so training a LoRA adapter
always happens externally, via Hugging Face `transformers`/`peft`/`trl`.

## Why pipx

This is packaged as an installable CLI app (`pyproject.toml` +
`aia-train-lora` entry point) rather than a loose `venv`/`requirements.txt`
checkout, specifically so it can be installed with [`pipx`][pipx]:

```bash
pipx install ./aia/training
```

`pipx` creates a fully isolated virtual environment for this app (with its
own `transformers`/`peft`/`trl`/`bitsandbytes`/`accelerate`/`datasets`
installs), and exposes only the `aia-train-lora` command on your `PATH` --
it never touches your system Python or any other project's environment, and
uninstalling later is a single `pipx uninstall aia-training`.

If you change `train_lora.py` or `pyproject.toml`, reinstall to pick up the
changes:

```bash
pipx install --force ./aia/training
```

`requirements.txt` is kept alongside for reference/reproducibility (pinned
versions), but `pyproject.toml` is what `pipx install` actually reads.

[pipx]: https://pipx.pypa.io/

## Recommended flow

1. **Export your data** from `aia`'s own `--session`/`--memory` files into a
   training-ready JSONL dataset (see the main `aia/README.md`):

   ```bash
   aia --export-training-data ./train.jsonl --export-source ./my-session.json --export-source ./my-memory.json
   ```

2. **Load the base model in 4-bit** (QLoRA) -- the *same* base model you run
   in `aia`, e.g. `meta-llama/Llama-3.2-1B-Instruct` -- via `bitsandbytes`,
   and attach a `peft.LoraConfig` (start with `r=16`, `alpha=32`, targeting
   `q_proj`/`v_proj`).

3. **Train** with `trl`'s `SFTTrainer` over the exported JSONL for a handful
   of epochs (start with 1-3 epochs on a small personal dataset; watch the
   training loss, don't overfit).

`train_lora.py` implements steps 2-3 as a concrete, runnable starting point:

```bash
aia-train-lora \
    --base-model meta-llama/Llama-3.2-1B-Instruct \
    --dataset ./train.jsonl \
    --output-dir ./my-adapter \
    --epochs 3
```

This writes `adapter_model.safetensors` + `adapter_config.json` into
`--output-dir`. As a sanity check, that file should be megabytes, not
gigabytes -- if it's not, something's off in the LoRA config (e.g. you
accidentally fine-tuned the whole model instead of just the adapter).

Key options (see `aia-train-lora --help` for the full list):

| Option | Default | Notes |
| --- | --- | --- |
| `--lora-r` | 16 | LoRA rank; higher = more capacity, more overfitting risk on tiny datasets |
| `--lora-alpha` | 32 | LoRA scaling factor |
| `--target-modules` | `q_proj v_proj` | Which linear layers get adapters |
| `--epochs` | 3 | Keep low for small personal datasets |

## Next steps

Once you have a trained adapter, convert it to GGUF and apply it via `aia`'s
`--lora`/`--lora-scale` flags -- see the "LoRA fine-tuning (personalization)"
section in `aia/README.md` for the exact commands and an A/B verification
workflow.
