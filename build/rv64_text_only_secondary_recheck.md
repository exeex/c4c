# RV64 Text-Only Secondary Recheck

Plan step: Step 4, "Recheck Secondary Representatives and Fail-Closed Behavior".

Command shape:

```sh
cmake --build --preset default
for n in 00024 00025 00045 00119; do
  ./build/c4cll --codegen asm --target riscv64-linux-gnu \
    "tests/c/external/c-testsuite/src/${n}.c" \
    -o "build/rv64_text_only_secondary_recheck/src_${n}.s"
  /usr/bin/clang --target=riscv64-linux-gnu --gcc-toolchain=/usr \
    -x assembler "build/rv64_text_only_secondary_recheck/src_${n}.s" \
    -o "build/rv64_text_only_secondary_recheck/src_${n}.bin" -lm
done
```

## Results

| Source | Feature family represented | Emitted asm contains `main`? | Link/next visible failure | Still silently succeeds with `.text` only? |
| --- | --- | --- | --- | --- |
| `src/00024.c` | aggregate global storage plus struct field stores | No. `src_00024.asm.head` contains only `.text`. | RISC-V linker reports undefined reference to `main`; feature-specific storage/lowering behavior remains hidden behind the missing entry point. | Yes. `c4cll` returns success and writes `.text` only. |
| `src/00025.c` | string literal storage plus external libc call to `strlen` | No. `src_00025.asm.head` contains only `.text`. | RISC-V linker reports undefined reference to `main`; string-literal and external-call lowering remain hidden behind the missing entry point. | Yes. `c4cll` returns success and writes `.text` only. |
| `src/00045.c` | initialized scalar globals plus pointer global initialization/deref | No. `src_00045.asm.head` contains only `.text`. | RISC-V linker reports undefined reference to `main`; scalar global, address-constant, and pointer-deref behavior remain hidden behind the missing entry point. | Yes. `c4cll` returns success and writes `.text` only. |
| `src/00119.c` | floating-point global storage and double comparison | No. `src_00119.asm.head` contains only `.text`. | RISC-V linker reports undefined reference to `main`; floating global and comparison behavior remain hidden behind the missing entry point. | Yes. `c4cll` returns success and writes `.text` only. |

## Classification

All four secondary representatives still reproduce the fail-open output-contract
mode: `c4cll` exits successfully, emits an assembly file whose first visible
content is only `.text`, and defers failure to the external link step as
`undefined reference to main`.

The no-storage focused repair is therefore visible for the Step 3 control case,
but these storage/external-call/floating representatives have not yet reached a
feature-specific assembler, linker, or runtime failure. Their next actionable
failure mode is still the missing `main` output contract, not the underlying
feature family.
