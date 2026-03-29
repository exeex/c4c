# Backend X86 External Call Object Plan

## Status

Open. This is the next bounded x86 backend child staged from the backend
umbrella roadmap.

## Goal

Keep `tests/c/internal/backend_case/call_helper.c` on the backend-owned x86
assembly/object path so the resulting object preserves an unresolved external
call relocation instead of falling back to LLVM text for that runtime case.

## Why This Slice Exists

The umbrella roadmap already closed the first backend-owned x86 assembler slice
for simpler runtime cases, but `call_helper.c` still marks the next missing
backend-owned mechanism boundary:

- the x86 runtime harness requests `BACKEND_OUTPUT_KIND=asm` for
  `call_helper.c`
- focused validation showed simpler cases such as `return_zero.c`,
  `return_add.c`, and `local_array.c` already stay on the backend-owned x86 asm
  path
- the missing behavior is the bounded direct external-call object contract, not
  broader x86 linker or assembler expansion

## Primary Scope

- `tests/c/internal/backend_case/call_helper.c`
- `tests/c/internal/InternalTests.cmake`
- `tests/c/internal/cmake/run_backend_contract_case.cmake`
- `src/backend/x86/codegen/emit.cpp`
- `src/backend/x86/codegen/calls.cpp`
- x86 backend-owned assembler/object emission surfaces only as required for this
  direct-call slice

## Expected Behavior

For an x86-64 Linux backend-owned asm/object build of `call_helper.c`:

- backend output should stay in native x86 assembly rather than LLVM IR
- the emitted assembly should contain a direct `call helper`
- the assembled object should preserve an unresolved external symbol for
  `helper`
- the relocation-bearing call site should be visible in `objdump -r`, most
  likely as `R_X86_64_PLT32`

## Non-Goals

- no broad x86 linker bring-up
- no general external symbol model beyond this direct helper-call case
- no unrelated backend runtime conversions
- no activation of the umbrella roadmap for direct implementation

## Guardrails

- keep the slice bounded to the single-function external direct-call contract
- prefer the smallest target-local change that preserves existing backend-owned
  x86 asm cases
- compare against Clang object behavior instead of guessing relocation details
- if execution discovers broader external-call/linker gaps, record them as a
  separate idea instead of silently expanding this plan

## Validation Intent

- add or enable a focused x86 backend contract test for `call_helper.c` object
  output
- keep the runtime case `backend_runtime_call_helper` on `BACKEND_OUTPUT_KIND=asm`
- verify emitted assembly and relocation snippets against Clang/objdump output
- run the targeted backend tests plus the full `ctest` suite before closure

## Handoff Notes

- this child replaces the temporary umbrella staging runbook as the active
  execution target
- `ideas/open/32_backend_builtin_assembler_x86_call_relocation_plan.md` remains
  deferred as a separate parked discovery note if later relocation-specific
  follow-up is still needed
