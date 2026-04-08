Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: A

Ownership:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/x86/codegen/emit.hpp`](/workspaces/c4c/src/backend/x86/codegen/emit.hpp) if needed

Goal:
- reduce x86 emitter-local `BackendModule` and `LirAdapterError` ownership
  until the remaining blockers to a BIR-native emitter are explicit and minimal

Priority kill list:
- replace `LirAdapterError` throws/catches with narrower local failure handling
  where the path is no longer really using shared legacy lowering semantics
- remove `#include "../../lowering/lir_to_backend_ir.hpp"` once file-local
  ownership no longer requires `LirAdapterError`
- if full removal is not yet possible, leave a smaller set of clearly bounded
  `BackendModule` helpers rather than a broad mixed-mode surface

Do:
- prefer deleting dead helper branches over preserving compatibility shims
- keep behavior on direct-LIR and BIR entry paths unchanged
- record any dependency on Group C instead of silently widening scope
- keep notes focused on what still blocks deleting the remaining x86
  `BackendModule` helpers

Do not edit:
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)
- [`todo.md`](/workspaces/c4c/todo.md), [`todoB.md`](/workspaces/c4c/todoB.md), [`todoC.md`](/workspaces/c4c/todoC.md), [`todoD.md`](/workspaces/c4c/todoD.md)

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/x86/codegen/emit.cpp.o`

Handoff standard:
- report which legacy includes/helpers/error paths were removed
- report the smallest remaining x86-only blocker if `BackendModule` or
  `LirAdapterError` ownership still remains

Status:
- third-wave slice completed and awaiting any fourth-wave reassignment
