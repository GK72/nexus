# Developer's Guide

Install CUDA toolkit and make sure it is added to PATH.

```sh
sudo pacman -S cuda
export PATH=/opt/cuda/bin:$PATH
```

Use the conversion tool provided by llama. Inject dependencies into the existing pipx environment

```
pipx inject aia-training gguf sentencepiece mistral_common
```
