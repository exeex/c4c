Status: Active
Source Idea Path: ideas/open/326_rv64_variadic_target_llvm_route_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Model RV64 Scalar stdarg As Pointer Cursor

# Current Packet

## Just Finished

Step 3 tightened RV64 scalar `stdarg` LLVM-route output as a pointer cursor while preserving AArch64 structured `va_list` behavior and Step 2 direct-call extension facts.

`src/codegen/shared/llvm_helpers.hpp` now treats RV64 targets as pointer-object `va_list` storage in the LLVM-route helper, so RV64 `va_list` locals lower as `ptr` storage and do not emit the AArch64-style `%struct.__va_list_tag_` declaration.

`src/codegen/lir/hir_to_lir/call/vaarg.cpp` now lowers only RV64 scalar or pointer `va_arg` payloads whose computed storage size fits one 8-byte variadic slot as cursor load, 8-byte inbounds `i8` cursor advance, cursor store, and load from the original cursor. Wider scalar payloads such as `long double` no longer enter the manual one-slot path and instead fall through to the previous pointer-object `va_arg` fallback.

Observed RV64 scalar `stdarg` IR for `build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll`:

- `%lv.ap = alloca ptr, align 8`
- `call void @llvm.va_start.p0(ptr %lv.ap)`
- `%t0 = load ptr, ptr %lv.ap`
- `%t1 = getelementptr inbounds i8, ptr %t0, i64 8`
- `store ptr %t1, ptr %lv.ap`
- `%t2 = load i32, ptr %t0`

Observed AArch64 scalar `stdarg` IR for `build/variadic_abi_triage/stdarg_scalar_int.c4c.aarch64.ll` still includes `%struct.__va_list_tag_`.

Observed Step 2 direct-call IR for `build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll` still includes:

- `declare signext i32 @printf(ptr, ...)`
- `%t1 = call signext i32 (ptr, ...) @printf(ptr %t0, i32 noundef signext 7)`

## Suggested Next

Execute Step 4 / Step 5 acceptance as the next packet:

Objective: promote the narrow RV64/AArch64 variadic LLVM-route facts into whatever regression or acceptance checkpoint the supervisor chooses, then decide whether the source idea can close or needs a follow-up for semantic BIR, prepared ABI, or final RV64 emission work.

Owned likely files depend on supervisor routing; if this remains implementation, likely test/fixture files plus `todo.md` and `test_after.log`.

Proof should include the full delegated Step 3 command or an equivalent acceptance subset covering:

- RV64 scalar `stdarg` pointer-cursor output with no `%struct.__va_list_tag_`.
- AArch64 scalar `stdarg` structured `%struct.__va_list_tag_` output.
- RV64 direct variadic `printf` return/call/argument `signext` facts.

## Watchouts

- Do not edit `ideas/open/326_rv64_variadic_target_llvm_route_facts.md` unless durable source intent changes.
- Do not weaken unsupported diagnostics or rewrite expectations as progress.
- Do not add `printf`-specific, fixture-shaped, or named-symbol shortcuts.
- Prefer target-owned LLVM-route/LIR facts before semantic BIR, prepared ABI, or final emission changes.
- RV64 scalar/pointer `va_arg` manual cursor lowering now advances by one 8-byte slot only for payloads whose computed storage size is <=8 bytes. Broader RV64 aggregate or >8-byte variadic payload support is intentionally not proven by this packet and must not be inferred from the current `int` proof.
- If Step 2/Step 3 are promoted into regression tests, include signed and unsigned RV64 variadic extern returns so declaration/call result attributes cannot diverge again, plus target-differentiated RV64/AArch64 scalar `stdarg` checks.

## Proof

Ran the delegated Step 3 proof successfully; output is preserved in `test_after.log`:

```sh
cmake --build --preset default && ./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && rg "%lv\.ap = alloca ptr" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && rg "call void @llvm\.va_start\.p0\(ptr %lv\.ap\)" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && rg "load ptr, ptr %lv\.ap" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && rg "getelementptr inbounds i8, ptr .*, i64 8" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && rg "store ptr .*, ptr %lv\.ap" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && rg "load i32, ptr" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && ! rg "%struct\.__va_list_tag_" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && ./build/c4cll --codegen llvm --target aarch64-linux-gnu build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.c4c.aarch64.ll && rg "%struct\.__va_list_tag_" build/variadic_abi_triage/stdarg_scalar_int.c4c.aarch64.ll && ./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/direct_variadic_printf.c -o build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll && rg "declare signext i32 @printf|call signext i32 .*@printf|i32 noundef signext 7" build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll
```
