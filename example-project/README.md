# Example project

This project demonstrates the usage of a dependency via Conan.

# Building

```sh
export NEXUS_PATH=$(git rev-parse --show-toplevel)
$NEXUS_PATH/tools/build -p $NEXUS_PATH/example-project -t install -- -DCMAKE_PREFIX_INSTALL=$NEXUS_PATH/example-project/env
```

To use static analysis, use the build script's flag.

For more information, see the script's help.

# Packaging

```sh
docker build $NEXUS_PATH/example-project/env --tag local/example:latest
```

# Test run

```
docker -it --rm local/example:latest /bin/example
```
