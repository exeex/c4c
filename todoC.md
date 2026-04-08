Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: C

Ownership:
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/lowering/call_decode.cpp`](/workspaces/c4c/src/backend/lowering/call_decode.cpp) if needed

Goal:
- remove the remaining shared lowering and decode ownership that only exists for
  legacy backend IR

Do:
- collapse dead compatibility shims
- keep surviving BIR routes explicit and small
- flag any emitter-local cleanup that belongs to Group A or B

Do not edit:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- parked legacy test files unless absolutely required
- [`todo.md`](/workspaces/c4c/todo.md), [`todoA.md`](/workspaces/c4c/todoA.md), [`todoB.md`](/workspaces/c4c/todoB.md), [`todoD.md`](/workspaces/c4c/todoD.md)

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/lir_to_backend_ir.cpp.o`
- or `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/call_decode.cpp.o`

Status:
- pending implementation
