#!/usr/bin/env bash
#
# Part of AIA (AI Assistant) Subproject.
#
# Downloads a GGUF model from Hugging Face into aia/models/ for use with the
# `aia` CLI. Ships a curated set of recommended small/instruct models as
# presets, but also accepts an arbitrary "<repo>/<file>" pair.
#
# Usage:
#   ./download-model.sh <preset|repo> [file] [output_dir]
#   ./download-model.sh --list
#
# Author: Gábor Krisztián Girhiny and Junie
# Date:   2026-07-14

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
OUTPUT_DIR="${3:-${SCRIPT_DIR}/models}"
HF_BASE_URL="https://huggingface.co"

# Recommended models (repo|file), picked for good quality-to-size ratio on a
# 16GB-VRAM GPU (RTX 5080) at Q4_K_M/Q5_K_M quantization:
#   - qwen2.5-3b     : strong general-purpose instruct model, good multilingual support.
#   - llama-3.2-3b   : Meta's small instruct model, well-documented and widely used.
#   - phi-3.5-mini   : Microsoft's 3.8B model, strong reasoning for its size.
#   - qwen2.5-1.5b   : tiny/fast option, good for quick experiments or CPU fallback.
#   - llama-3.2-1b   : smallest option, useful as a sanity-check baseline.
declare -A PRESETS=(
    [qwen2.5-3b]="Qwen/Qwen2.5-3B-Instruct-GGUF|qwen2.5-3b-instruct-q4_k_m.gguf"
    [llama-3.2-3b]="bartowski/Llama-3.2-3B-Instruct-GGUF|Llama-3.2-3B-Instruct-Q4_K_M.gguf"
    [phi-3.5-mini]="bartowski/Phi-3.5-mini-instruct-GGUF|Phi-3.5-mini-instruct-Q4_K_M.gguf"
    [qwen2.5-1.5b]="Qwen/Qwen2.5-1.5B-Instruct-GGUF|qwen2.5-1.5b-instruct-q4_k_m.gguf"
    [llama-3.2-1b]="bartowski/Llama-3.2-1B-Instruct-GGUF|Llama-3.2-1B-Instruct-Q4_K_M.gguf"
)

usage() {
    echo "Usage: $(basename "$0") <preset|hf_repo> [file] [output_dir]"
    echo "       $(basename "$0") --list"
    echo ""
    echo "Arguments:"
    echo "  preset|hf_repo   One of the recommended presets below, or an arbitrary"
    echo "                   Hugging Face repo (e.g. 'someuser/some-model-GGUF')."
    echo "  file             GGUF filename within the repo. Required when 'hf_repo'"
    echo "                   is not a preset; ignored for presets."
    echo "  output_dir       Directory to download into (default: aia/models)."
    echo ""
    echo "Recommended presets:"
    for name in "${!PRESETS[@]}"; do
        echo "  ${name}"
    done | sort
}

list_presets() {
    echo "Recommended presets (repo -> file):"
    for name in "${!PRESETS[@]}"; do
        IFS='|' read -r repo file <<< "${PRESETS[$name]}"
        printf "  %-14s %s / %s\n" "${name}" "${repo}" "${file}"
    done | sort
}

if [[ $# -eq 0 || "${1}" == "-h" || "${1}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ "${1}" == "--list" ]]; then
    list_presets
    exit 0
fi

PRESET_OR_REPO="${1}"

if [[ -n "${PRESETS[${PRESET_OR_REPO}]+_}" ]]; then
    IFS='|' read -r REPO FILE <<< "${PRESETS[${PRESET_OR_REPO}]}"
else
    REPO="${PRESET_OR_REPO}"
    FILE="${2:-}"
    if [[ -z "${FILE}" ]]; then
        echo "Error: '${PRESET_OR_REPO}' is not a known preset, and no 'file' argument was given." >&2
        echo "" >&2
        usage
        exit 1
    fi
fi

mkdir -p "${OUTPUT_DIR}"
OUTPUT_PATH="${OUTPUT_DIR}/${FILE}"
DOWNLOAD_URL="${HF_BASE_URL}/${REPO}/resolve/main/${FILE}"

echo "Downloading ${REPO} / ${FILE}"
echo "  -> ${OUTPUT_PATH}"

curl -L --fail --continue-at - -o "${OUTPUT_PATH}" "${DOWNLOAD_URL}"

echo "Done. Run it with:"
echo "  aia --model \"${OUTPUT_PATH}\" --prompt \"Hello, \""
