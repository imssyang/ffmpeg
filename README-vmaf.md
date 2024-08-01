# Mac

```bash
# Dependencies
brew install meson ninja nasm

# Config
meson setup build --prefix /usr/local \
  --buildtype release \
  -Denable_float=true \
  -Denable_avx512=true

# Compile
ninja -vC build

# Test
ninja -vC build test

# Install
ninja -vC build install
```

[Netflix/vmaf](https://github.com/Netflix/vmaf)

