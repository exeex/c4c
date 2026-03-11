# Known Issues

## Clang/GCC Divergence On `bitfld-3.c` And `bitfld-5.c`

### Affected cases

- `llvm_gcc_c_torture_bitfld_3_c`
- `llvm_gcc_c_torture_bitfld_5_c`

### Environment observed

- host arch: `aarch64`
- date verified: `2026-03-11`

### Current behavior

These two gcc torture tests are not currently useful as c4cll regressions on this
machine because the host reference compilers disagree:

- `clang` compiles both cases but the produced binaries abort at runtime
- `gcc` compiles both cases and the produced binaries pass

This matches the local test-suite configuration in
`tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/CMakeLists.txt`,
which already lists `bitfld-3.c` and `bitfld-5.c` under "Tests where clang currently
has bugs or issues".

### Reproduction

```bash
clang tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/bitfld-3.c -o /tmp/clang-bitfld-3.bin && /tmp/clang-bitfld-3.bin
gcc   tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/bitfld-3.c -o /tmp/gcc-bitfld-3.bin && /tmp/gcc-bitfld-3.bin

clang tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/bitfld-5.c -o /tmp/clang-bitfld-5.bin && /tmp/clang-bitfld-5.bin
gcc   tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/bitfld-5.c -o /tmp/gcc-bitfld-5.bin && /tmp/gcc-bitfld-5.bin
```

Expected locally:

- `clang bitfld-3`: aborts
- `gcc bitfld-3`: exits `0`
- `clang bitfld-5`: aborts
- `gcc bitfld-5`: exits `0`

### Practical guidance

Do not treat `bitfld-3` and `bitfld-5` failures as evidence of a new c4cll bitfield
regression unless the same behavior is first reproduced against the chosen reference
compiler for the target environment.
