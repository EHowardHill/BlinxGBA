# BlinxGBA

## Build Process

1. Set up devkitPro based on [these instructions](https://devkitpro.org/wiki/Getting_Started).

2. Set up Butano by pulling it from its submodule:
`git submodule update --init`

3. Build using make:
`make -j$(nproc)`