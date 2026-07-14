#!/usr/bin/env python3
"""External LoRA/QLoRA training script for AIA.

This is intentionally *outside* aia's C++ build (llama.cpp's C API has no
training/backward pass): it loads a base model from Hugging Face in 4-bit
(QLoRA), attaches a small trainable LoRA adapter, and fine-tunes it on a
JSONL dataset produced by `aia --export-training-data`. The resulting
adapter is later converted to GGUF (llama.cpp's convert_lora_to_gguf.py)
and applied at inference time via aia's --lora/--lora-scale flags.

Install and run via pipx (recommended, keeps this fully isolated from any
other Python environment on the machine):

    pipx install ./aia/training
    aia-train-lora --base-model meta-llama/Llama-3.2-1B-Instruct \\
        --dataset ./train.jsonl --output-dir ./my-adapter

See aia/training/README.md for the full flow (export -> train -> convert -> apply).
"""

import argparse

from datasets import load_dataset
from peft import LoraConfig
from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig
from trl import SFTConfig, SFTTrainer


def parse_args() -> argparse.Namespace:
    """Parse command line arguments for the training run."""
    parser = argparse.ArgumentParser(description="Train a LoRA/QLoRA adapter on an aia-exported JSONL dataset")
    parser.add_argument("--base-model", required=True, help="Hugging Face repo id of the base model (the same one used by aia, e.g. meta-llama/Llama-3.2-1B-Instruct)")
    parser.add_argument("--dataset", required=True, help="Path to the JSONL dataset produced by `aia --export-training-data`")
    parser.add_argument("--output-dir", default="./lora-out", help="Directory to write the trained adapter (adapter_model.safetensors + adapter_config.json)")
    parser.add_argument("--lora-r", type=int, default=16, help="LoRA rank")
    parser.add_argument("--lora-alpha", type=int, default=32, help="LoRA alpha (scaling factor)")
    parser.add_argument("--lora-dropout", type=float, default=0.05, help="LoRA dropout")
    parser.add_argument("--target-modules", nargs="+", default=["q_proj", "v_proj"], help="Linear layers to attach LoRA adapters to")
    parser.add_argument("--epochs", type=float, default=3.0, help="Number of training epochs (start small on a small personal dataset)")
    parser.add_argument("--batch-size", type=int, default=1, help="Per-device training batch size")
    parser.add_argument("--learning-rate", type=float, default=2e-4, help="Learning rate")
    return parser.parse_args()


def main() -> None:
    """Load the base model in 4-bit, attach a LoRA adapter, and run SFTTrainer over the exported dataset."""
    args = parse_args()

    bnb_config = BitsAndBytesConfig(
        load_in_4bit=True,
        bnb_4bit_quant_type="nf4",
        bnb_4bit_use_double_quant=True,
        bnb_4bit_compute_dtype="bfloat16",
    )

    tokenizer = AutoTokenizer.from_pretrained(args.base_model)
    model = AutoModelForCausalLM.from_pretrained(
        args.base_model,
        quantization_config=bnb_config,
        device_map="auto",
    )

    lora_config = LoraConfig(
        r=args.lora_r,
        lora_alpha=args.lora_alpha,
        lora_dropout=args.lora_dropout,
        target_modules=args.target_modules,
        task_type="CAUSAL_LM",
    )

    dataset = load_dataset("json", data_files=args.dataset, split="train")

    sft_config = SFTConfig(
        output_dir=args.output_dir,
        num_train_epochs=args.epochs,
        per_device_train_batch_size=args.batch_size,
        learning_rate=args.learning_rate,
        logging_steps=1,
    )

    trainer = SFTTrainer(
        model=model,
        args=sft_config,
        train_dataset=dataset,
        peft_config=lora_config,
        processing_class=tokenizer,
    )

    trainer.train()
    trainer.save_model(args.output_dir)


if __name__ == "__main__":
    main()
