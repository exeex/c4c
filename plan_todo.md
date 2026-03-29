# x86 Backend Bring-Up Todo

Status: Active
Source Idea: ideas/open/15_backend_x86_port_plan.md
Source Plan: plan.md

## Current Active Item

- [ ] Step 3: replace the staged x86 LLVM passthrough seam with a minimal assembly emit path for direct integer returns

## Ordered Checklist

- [x] Step 1: audit and normalize the mirrored x86 tree
- [x] Step 2: make top-level x86 entry surfaces include-clean
- [ ] Step 3: reconnect the backend driver through a narrow x86 adapter
- [ ] Step 4: land the first minimal emit slice for a trivial integer-return function
- [ ] Step 5: record remaining gaps and the next slice

## Next Intended Slice

Keep the explicit `backend -> x86::emit_module` seam, then swap the current LLVM-text passthrough for a true x86 assembly slice that handles one trivial integer-return function.

## Blockers

- None recorded yet.

## Resume Notes

- Audit result: translated `.cpp` file coverage already matches the ref `.rs` tree for the x86 backend code paths used in this plan; the first real blocker was missing top-level headers and build participation, not missing mirrored source files.
- `src/backend/x86/mod.cpp`, `src/backend/x86/assembler/mod.cpp`, `src/backend/x86/codegen/mod.cpp`, `src/backend/x86/codegen/emit.cpp`, and `src/backend/x86/linker/mod.cpp` now compile in both `c4cll` and `backend_lir_adapter_tests`.
- The backend driver now routes `Target::X86_64` and `Target::I686` through `c4c::backend::x86::emit_module`, but that seam still returns LLVM-style LIR text as a staging behavior.
- `tests/backend/backend_lir_adapter_tests.cpp` now has a focused x86 seam test plus include-reachability checks for the staged x86 assembler/linker headers.
- Keep the x86 tree structurally close to `ref/claudes-c-compiler/src/backend/x86/`.
- Do not absorb assembler or linker bring-up unless the minimal codegen slice proves it is necessary.
- If adjacent x86 work appears but is not required for the trivial emit path, record it in the source idea instead of broadening the active plan.
