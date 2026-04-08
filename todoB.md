Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: B

Ownership:
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/aarch64/codegen/emit.hpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.hpp) if needed

Goal:
- reduce aarch64 emitter-local `BackendModule` ownership until the remaining
  blockers to a BIR-native emitter are explicit and minimal

Priority kill list:
- remove dead internal legacy `BackendModule` entry/helper code if it is no
  longer reachable
- remove `#include "../../lowering/lir_to_backend_ir.hpp"` once file-local
  ownership no longer requires it
- remove `#include "../../ir_printer.hpp"` and
  `#include "../../ir_validate.hpp"` once file-local ownership no longer
  requires them
- if full removal is not yet possible, leave a smaller set of clearly bounded
  `BackendModule` helpers rather than a broad mixed-mode surface

Do:
- prefer deleting dead helper branches over preserving compatibility shims
- keep behavior on direct-LIR and BIR entry paths unchanged
- record any dependency on Group C instead of silently widening scope
- keep notes focused on what still blocks deleting the remaining aarch64
  `BackendModule` helpers

Do not edit:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)
- [`todo.md`](/workspaces/c4c/todo.md), [`todoA.md`](/workspaces/c4c/todoA.md), [`todoC.md`](/workspaces/c4c/todoC.md), [`todoD.md`](/workspaces/c4c/todoD.md)

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/aarch64/codegen/emit.cpp.o`

Handoff standard:
- report which legacy includes/helpers were removed
- report the smallest remaining aarch64-only blocker if `BackendModule`
  ownership still remains

Status:
- second-wave slice completed and awaiting any third-wave reassignment
