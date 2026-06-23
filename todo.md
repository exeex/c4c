Status: Active
Source Idea Path: ideas/open/325_variadic_target_ir_abi_boundary_triage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Prepare Minimal Evidence Fixtures

# Current Packet

## Just Finished

Step 1: Prepare Minimal Evidence Fixtures completed by creating
`build/variadic_abi_triage/direct_variadic_printf.c`,
`build/variadic_abi_triage/stdarg_scalar_int.c`, and
`build/variadic_abi_triage/notes.md`. The notes name the
`aarch64-linux-gnu` and `riscv64-linux-gnu` target triples and explain why the
direct external variadic call and scalar stdarg fixtures are included.

## Suggested Next

Begin Step 2 by capturing clang LLVM IR for both fixtures under
`build/variadic_abi_triage/` for `aarch64-linux-gnu` and
`riscv64-linux-gnu`, then record the direct variadic call and scalar stdarg ABI
differences in the triage notes.

## Watchouts

There is no standalone `--dump-lir` in the current `c4cll` CLI. Use
`--codegen llvm` for LLVM-route evidence, then `--dump-bir` and
`--dump-prepared-bir` for backend evidence. Keep RV64 variadic diagnostics
fail-closed during this triage. No aggregate or floating `va_arg` fixture was
added in Step 1; add one only if scalar evidence leaves a specific ABI-boundary
question unanswered.

## Proof

Proof command:

```sh
test -f build/variadic_abi_triage/direct_variadic_printf.c && test -f build/variadic_abi_triage/stdarg_scalar_int.c && test -f build/variadic_abi_triage/notes.md && rg 'aarch64-linux-gnu|riscv64-linux-gnu|direct external variadic|scalar stdarg' build/variadic_abi_triage/notes.md
```

Result: passed. The supervisor-selected proof is sufficient for this
fixture-and-notes packet. Proof log: `test_after.log`.
