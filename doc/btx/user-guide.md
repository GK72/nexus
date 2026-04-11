# BTX User Guide

The BTX tool allows converting between binary and BTX text format.

## Usage Examples

<!-- TODO: Have the tool installed by CMake and refer to that workflow instead of `build/debug` path -->

### Convert BTX text to binary:
```bash
./build/debug/btx/btx-tool input.btx -o output.bin
```

### Convert binary to BTX text:
```bash
./build/debug/btx/btx-tool -f input.bin -o output.btx
```

### Convert binary to annotated BTX text using a descriptor:
```bash
./build/debug/btx/btx-tool -f input.bin -d res/descriptor.yaml -o output.btx
```
