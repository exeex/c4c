Status: Active
Source Idea Path: ideas/open/326_rv64_variadic_target_llvm_route_facts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Acceptance Checkpoint

# Current Packet

## Just Finished

Step 4 / Step 5 acceptance promoted the accepted RV64/AArch64 variadic LLVM-route facts into focused regression coverage in `tests/frontend/frontend_lir_call_type_ref_test.cpp`.

The existing `frontend_lir_call_type_ref` binary now asserts:

- RV64 direct variadic signed extern returns carry `signext` on declaration and call result.
- RV64 direct variadic unsigned extern returns carry `zeroext` on declaration and call result.
- RV64 direct variadic integer arguments carry `noundef signext`.
- RV64 scalar `stdarg` lowers `va_list` as pointer storage, calls `llvm.va_start.p0(ptr %lv.ap)`, advances the cursor by one 8-byte slot, stores the advanced cursor, loads the scalar from the original cursor, and does not emit `%struct.__va_list_tag_`.
- AArch64 scalar `stdarg` still preserves the structured `%struct.__va_list_tag_` route.

## Suggested Next

Request plan-owner closure review for `ideas/open/326_rv64_variadic_target_llvm_route_facts.md`. The Step 4 / Step 5 acceptance checkpoint is complete from the executor side.

## Watchouts

- This acceptance coverage is LLVM-route focused. It does not prove semantic BIR, prepared ABI, final RV64 emission, RV64 aggregate `va_arg`, or wider-than-8-byte RV64 `va_arg` payload support.
- The test fixture uses `typedef __builtin_va_list va_list;` instead of `#include <stdarg.h>` because `frontend_lir_call_type_ref_test` lowers in-process parser fixtures without the file preprocessor.
- No implementation source files, unsupported diagnostics, expectation downgrades, `plan.md`, or source idea files were touched in this packet.

## Proof

Ran the delegated Step 4 / Step 5 acceptance proof successfully; output is preserved in `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' && ./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && rg "%lv\.ap = alloca ptr" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && rg "call void @llvm\.va_start\.p0\(ptr %lv\.ap\)" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && rg "getelementptr inbounds i8, ptr .*, i64 8" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && ! rg "%struct\.__va_list_tag_" build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll && ./build/c4cll --codegen llvm --target aarch64-linux-gnu build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.c4c.aarch64.ll && rg "%struct\.__va_list_tag_" build/variadic_abi_triage/stdarg_scalar_int.c4c.aarch64.ll && ./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/direct_variadic_printf.c -o build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll && rg "declare signext i32 @printf|call signext i32 .*@printf|i32 noundef signext 7" build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll
```
