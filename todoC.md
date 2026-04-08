Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: C

Ownership:
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/lowering/call_decode.cpp`](/workspaces/c4c/src/backend/lowering/call_decode.cpp) if needed
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp) if needed
- [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp) and directly adjacent BIR helpers
  only if a small parity addition is clearly bounded

Goal:
- turn the shared legacy seam into an explicit BIR parity map, beginning with
  `call_decode` and the remaining `BackendModule` parser surface

Priority parity map:
- identify which `parse_backend_minimal_*_module(...)` helpers truly require
  `BackendModule`
- separate "needs new BIR field/type" from "needs consumer rewrite"
- if a tiny BIR-facing helper or shape addition is obvious and safe, land it
  in the same slice

Do:
- keep surviving bridge code explicit and temporary
- avoid drifting into emitter-local deep refactors owned by Group A or B
- prefer clarifying the replacement contract over prematurely deleting legacy
  files

Do not edit:
- deep x86/aarch64 emitter logic except for coordinated signature fallout
- `ir.*` physical deletion in this round
- [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)
- [`todo.md`](/workspaces/c4c/todo.md), [`todoA.md`](/workspaces/c4c/todoA.md), [`todoB.md`](/workspaces/c4c/todoB.md), [`todoD.md`](/workspaces/c4c/todoD.md)

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/call_decode.cpp.o`
- or `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/lir_to_backend_ir.cpp.o`

Handoff standard:
- report which shared helpers still require `BackendModule`
- report which gaps are true missing BIR shape
- report any bounded BIR addition landed in this slice

Status:
- ready for BIR parity wave
