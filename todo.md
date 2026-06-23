Status: Active
Source Idea Path: ideas/open/325_variadic_target_ir_abi_boundary_triage.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Capture Clang Target LLVM IR Evidence

# Current Packet

## Just Finished

Step 2: Capture Clang Target LLVM IR Evidence completed by generating clang
LLVM IR for both variadic triage fixtures under `aarch64-linux-gnu` and
`riscv64-linux-gnu`:
`build/variadic_abi_triage/direct_variadic_printf.aarch64.ll`,
`build/variadic_abi_triage/direct_variadic_printf.rv64.ll`,
`build/variadic_abi_triage/stdarg_scalar_int.aarch64.ll`, and
`build/variadic_abi_triage/stdarg_scalar_int.rv64.ll`. The triage notes now
record concrete AArch64 versus RV64 differences for the direct external
variadic call and the scalar `llvm.va_start`/`va_arg` path.

## Suggested Next

Begin Step 3 by comparing c4cll LLVM-route output against the clang IR
artifacts for the same fixtures and targets, keeping the comparison focused on
the direct variadic call attributes and scalar `va_list`/`llvm.va_start`
lowering differences.

## Watchouts

There is no standalone `--dump-lir` in the current `c4cll` CLI. Use
`--codegen llvm` for LLVM-route evidence, then `--dump-bir` and
`--dump-prepared-bir` for backend evidence. Keep RV64 variadic diagnostics
fail-closed during this triage. The clang IR shows RV64 `signext` attributes
on `i32` returns and variadic integer operands, plus a pointer-cursor
`va_list`; AArch64 uses a structured `%struct.__va_list` with register-save
offset handling and no equivalent `signext` on the minimal integer call.

## Proof

Proof command:

```sh
clang --target=aarch64-linux-gnu -S -emit-llvm build/variadic_abi_triage/direct_variadic_printf.c -o build/variadic_abi_triage/direct_variadic_printf.aarch64.ll && clang --target=riscv64-linux-gnu -S -emit-llvm build/variadic_abi_triage/direct_variadic_printf.c -o build/variadic_abi_triage/direct_variadic_printf.rv64.ll && clang --target=aarch64-linux-gnu -S -emit-llvm build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.aarch64.ll && clang --target=riscv64-linux-gnu -S -emit-llvm build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.rv64.ll && test -s build/variadic_abi_triage/direct_variadic_printf.aarch64.ll && test -s build/variadic_abi_triage/direct_variadic_printf.rv64.ll && test -s build/variadic_abi_triage/stdarg_scalar_int.aarch64.ll && test -s build/variadic_abi_triage/stdarg_scalar_int.rv64.ll && rg 'AArch64|RV64|va_list|llvm\.va_start|direct external variadic|stdarg' build/variadic_abi_triage/notes.md
```

Result: passed. The supervisor-selected proof is sufficient for this clang IR
evidence packet. Proof log: `test_after.log`.
