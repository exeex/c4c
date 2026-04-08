Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: C

Ownership:
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/lowering/call_decode.cpp`](/workspaces/c4c/src/backend/lowering/call_decode.cpp) if needed
- directly adjacent shared backend headers only if required by the same seam
  reduction

Goal:
- shrink the shared lowering surface so legacy backend IR looks temporary and
  exceptional instead of like a normal backend API

Priority kill list:
- reduce public exposure of `lower_lir_to_backend_module(...)`
- remove dead compatibility helpers that only serve legacy backend IR
- keep `call_decode` ownership local to implementation files where possible
- avoid widening into emitter-local cleanup that belongs to Group A or B

Do:
- collapse dead compatibility shims
- keep surviving BIR routes explicit and small
- leave any still-needed bridge code clearly temporary
- report the narrowest remaining blocker to deleting `lir_to_backend_ir.*`

Do not edit:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- parked legacy test files unless directly required by the same API reduction
- [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)
- [`todo.md`](/workspaces/c4c/todo.md), [`todoA.md`](/workspaces/c4c/todoA.md), [`todoB.md`](/workspaces/c4c/todoB.md), [`todoD.md`](/workspaces/c4c/todoD.md)

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/lir_to_backend_ir.cpp.o`
- or `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/call_decode.cpp.o`

Handoff standard:
- report which public/compat surface was removed or narrowed
- report the smallest remaining blocker to deleting or fully internalizing
  `lir_to_backend_ir.*`

Status:
- second-wave slice completed and awaiting any third-wave reassignment
