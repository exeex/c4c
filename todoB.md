Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: B

Ownership:
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/aarch64/codegen/emit.hpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.hpp) if needed

Goal:
- remove or isolate the remaining aarch64 `BackendModule` helper ownership now
  that the public entry is already hard-locked

Do:
- prefer deleting dead helper branches over preserving compatibility shims
- keep behavior on direct-LIR and BIR entry paths unchanged
- record any dependency on Group C instead of silently widening scope

Do not edit:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- [`todo.md`](/workspaces/c4c/todo.md), [`todoA.md`](/workspaces/c4c/todoA.md), [`todoC.md`](/workspaces/c4c/todoC.md), [`todoD.md`](/workspaces/c4c/todoD.md)

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/aarch64/codegen/emit.cpp.o`

Status:
- pending implementation
