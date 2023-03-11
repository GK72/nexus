# Example project

This project demonstrates the usage of a dependency via Conan.

# Building

```sh
export NEXUS_PATH=$(git rev-parse --show-toplevel)
$NEXUS_PATH/tools/build -p $NEXUS_PATH/example-project -- -DCMAKE_INSTALL_PREFIX=$NEXUS_PATH/example-project/env
$NEXUS_PATH/tools/build -p $NEXUS_PATH/example-project -t install
```

To use static analysis, use the build script's flag.

For more information, see the script's help.

# Packaging

```sh
cd $NEXUS_PATH/example-project/env
docker build . --tag local/example:latest
```

# Test run

```
docker run -it --rm local/example:latest /bin/example
```
