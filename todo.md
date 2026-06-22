Status: Active
Source Idea Path: ideas/open/307_rv64_text_only_fail_closed_output_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Recheck Secondary Representatives and Fail-Closed Behavior

# Current Packet

## Just Finished

Completed `plan.md` Step 4, "Recheck Secondary Representatives and Fail-Closed
Behavior": regenerated RV64 asm/link scratch artifacts for secondary
representatives `src/00024.c`, `src/00025.c`, `src/00045.c`, and `src/00119.c`
under `build/rv64_text_only_secondary_recheck/`, and recorded the results in
`build/rv64_text_only_secondary_recheck.md`.

All four cases still emit `.text` only with no `.globl main` or `main:` label,
then fail at the RISC-V link step with undefined reference to `main`. No case
has advanced to a feature-specific assembler, linker, or runtime failure yet;
each still silently succeeds from `c4cll` with a text-only output file.

## Suggested Next

Continue Step 4 with a bounded fail-closed output-contract repair: unsupported
prepared global storage/addressing cases must not return successful `.text`
only before function emission. Keep the repair to reporting or rejecting
unsupported prepared-module input honestly; do not implement aggregate, string,
pointer, floating, scalar-global, libc-call, or full c-testsuite feature
support in this slice.

## Watchouts

- `src/00024.c` represents aggregate global storage and struct field stores.
- `src/00025.c` represents string literal storage plus an external libc call.
- `src/00045.c` represents initialized scalar globals, pointer global
  initialization, and pointer dereference.
- `src/00119.c` represents floating global storage and double comparison.
- Do not implement secondary feature repairs as part of this recheck packet.
- Do not treat the four text-only successes as supported outputs; they remain
  fail-open output-contract cases.

## Proof

Proof command delegated by the supervisor:

```sh
{ cmake --build --preset default && mkdir -p build/rv64_text_only_secondary_recheck && for n in 00024 00025 00045 00119; do src="tests/c/external/c-testsuite/src/${n}.c"; asm="build/rv64_text_only_secondary_recheck/src_${n}.s"; bin="build/rv64_text_only_secondary_recheck/src_${n}.bin"; ./build/c4cll --codegen asm --target riscv64-linux-gnu "$src" -o "$asm" > "build/rv64_text_only_secondary_recheck/src_${n}.emit.out" 2>&1 || true; sed -n '1,20p' "$asm" > "build/rv64_text_only_secondary_recheck/src_${n}.asm.head" 2>/dev/null || true; /usr/bin/clang --target=riscv64-linux-gnu --gcc-toolchain=/usr -x assembler "$asm" -o "$bin" -lm > "build/rv64_text_only_secondary_recheck/src_${n}.clang.out" 2>&1 || true; done; test -f build/rv64_text_only_secondary_recheck.md && rg -n '00024|00025|00045|00119|\.globl main|main:|next failure|unsupported|link|assembler|runtime|feature' build/rv64_text_only_secondary_recheck.md build/rv64_text_only_secondary_recheck/*.asm.head build/rv64_text_only_secondary_recheck/*.clang.out; } > test_after.log 2>&1
```

Result: passed with exit code 0. Build was up to date, all four c4cll emits
returned successfully, all four asm heads remained `.text` only, all four
clang link attempts reported undefined reference to `main`, and the final `rg`
check found the recorded representatives and failure classifications. Proof
log: `test_after.log`.
