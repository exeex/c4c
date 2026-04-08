Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: C

Ownership:
- [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp)
- [`src/backend/bir.cpp`](/workspaces/c4c/src/backend/bir.cpp) if needed
- [`src/backend/bir_printer.hpp`](/workspaces/c4c/src/backend/bir_printer.hpp) / [`src/backend/bir_printer.cpp`](/workspaces/c4c/src/backend/bir_printer.cpp) if needed
- [`src/backend/bir_validate.hpp`](/workspaces/c4c/src/backend/bir_validate.hpp) / [`src/backend/bir_validate.cpp`](/workspaces/c4c/src/backend/bir_validate.cpp) if needed
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/lowering/call_decode.cpp`](/workspaces/c4c/src/backend/lowering/call_decode.cpp) if needed
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp) /
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp) if needed

Goal:
- land the first bounded BIR contract additions for call / extern / global /
  string / local-slot shape

Priority contract work:
- choose the smallest useful subset of those gaps and model it in BIR
- keep the addition migration-oriented rather than trying to finish all parity
  at once
- connect the new shape explicitly to the remaining `BackendModule` seam in
  `call_decode` or adjacent shared helpers

Do:
- prefer one or two bounded contract additions over a sprawling schema rewrite
- keep bridge code explicit and temporary
- avoid deep emitter-local refactors owned by Group A or B

Do not edit:
- deep x86/aarch64 emitter logic except for coordinated fallout from a shared
  signature change
- `ir.*` physical deletion in this round
- [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)
- [`todo.md`](/workspaces/c4c/todo.md), [`todoA.md`](/workspaces/c4c/todoA.md), [`todoB.md`](/workspaces/c4c/todoB.md), [`todoD.md`](/workspaces/c4c/todoD.md)

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/call_decode.cpp.o`
- or `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/lir_to_backend_ir.cpp.o`

Handoff standard:
- report which BIR contract addition landed
- report which remaining `BackendModule` helpers it is intended to unlock
- report the next most obvious missing BIR contract after this slice

Status:
- ready for BIR contract expansion wave
