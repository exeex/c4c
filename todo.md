Status: Active
Source Idea Path: ideas/open/326_rv64_variadic_target_llvm_route_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish RV64 Direct Variadic Integer Extension Facts

# Current Packet

## Just Finished

Step 2 fixed the RV64 direct external variadic declaration return-attribute generality bug without changing `stdarg` lowering.

`src/codegen/lir/hir_to_lir/hir_to_lir.cpp` now chooses extern variadic declaration return extension attributes from the actual return `TypeSpec`: signed 32-bit integer returns render `signext`, unsigned 32-bit integer returns render `zeroext`, and non-matching returns render no attribute. This keeps declaration spelling aligned with the existing call-result attribute path instead of treating every RV64 variadic `i32` declaration as signed.

Observed Step 2 IR for `build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll`:

- `declare signext i32 @printf(ptr, ...)`
- `%t1 = call signext i32 (ptr, ...) @printf(ptr %t0, i32 noundef signext 7)`

Observed local unsigned-return probe IR for `build/variadic_abi_triage/rv64_unsigned_variadic_return_probe.c4c.rv64.ll`:

- `declare zeroext i32 @vf(ptr, ...)`
- `%t1 = call zeroext i32 (ptr, ...) @vf(ptr %t0, i32 noundef signext 7)`

## Suggested Next

Execute Step 3 as the next packet:

Objective: model RV64 scalar `stdarg` as pointer-cursor LLVM-route output while preserving the existing AArch64 structured `%struct.__va_list` route.

Owned likely implementation files: `src/codegen/shared/llvm_helpers.hpp`, `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`, `src/codegen/lir/hir_to_lir/call/vaarg.cpp`, `src/codegen/lir/lir_printer.cpp`, `todo.md`, `test_after.log`, and refreshed triage IR files for RV64/AArch64 scalar `stdarg`.

Proof command:

```sh
cmake --build --preset default && \
./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && \
rg "llvm.va_start|load ptr|store ptr|getelementptr inbounds i8, ptr .*, i64 8|load i32" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll
```

## Watchouts

- Do not edit `ideas/open/326_rv64_variadic_target_llvm_route_facts.md` unless durable source intent changes.
- Do not weaken unsupported diagnostics or rewrite expectations as progress.
- Do not add `printf`-specific, fixture-shaped, or named-symbol shortcuts.
- Keep AArch64 structured `%struct.__va_list` behavior intact.
- Prefer target-owned LLVM-route/LIR facts before semantic BIR, prepared ABI, or final emission changes.
- Step 2 intentionally did not change `stdarg`; `build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll` still contains the current `%struct.__va_list_tag_` type declaration because Step 3 owns that route.
- Step 3 should likely change `llvm_va_list_is_pointer_object`/storage policy for RV64 plus add an explicit RV64 pointer-cursor `va_arg` lowering instead of routing RV64 through `LirVaArgOp` if the desired IR must expose cursor load, 8-byte GEP, cursor store, and scalar load.
- If Step 2 is promoted into a regression test, include both signed and unsigned RV64 variadic extern returns so declaration/call result attributes cannot diverge again.

## Proof

Ran the delegated Step 2 proof successfully and appended the local unsigned-return probe; output is preserved in `test_after.log`:

```sh
cmake --build --preset default && \
./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/direct_variadic_printf.c -o build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll && \
rg "declare signext i32 @printf|call signext i32 .*@printf|i32 noundef signext 7" build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll
```

Additional unsigned-return probe:

```sh
printf '%s\n' 'extern unsigned int vf(const char *, ...); int main(void) { return (int)vf("%d", 7); }' > build/variadic_abi_triage/rv64_unsigned_variadic_return_probe.c
./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/rv64_unsigned_variadic_return_probe.c -o build/variadic_abi_triage/rv64_unsigned_variadic_return_probe.c4c.rv64.ll
rg "declare zeroext i32 @vf|call zeroext i32 .*@vf|i32 noundef signext 7" build/variadic_abi_triage/rv64_unsigned_variadic_return_probe.c4c.rv64.ll
! rg "declare signext i32 @vf|call signext i32 .*@vf" build/variadic_abi_triage/rv64_unsigned_variadic_return_probe.c4c.rv64.ll
```
