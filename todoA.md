Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: A

Ownership:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/x86/codegen/emit.hpp`](/workspaces/c4c/src/backend/x86/codegen/emit.hpp) if needed

Goal:
- inventory the remaining x86 emitter-side `BackendModule` dependency and make
  the missing BIR replacement shape explicit

Priority parity map:
- group the remaining x86 `BackendModule` helpers by semantic role
- separate "pure symbol/plumbing" from "real module/function metadata"
- identify which remaining helpers could switch to BIR with existing shape and
  which require new BIR fields or helper abstractions

Do:
- prefer bounded annotations, helper clustering, or tiny clarifying refactors
  over broad rewrites
- keep behavior on direct-LIR and BIR entry paths unchanged
- record any dependency on Group C when the missing piece is really shared BIR
  shape or `call_decode` contract

Do not edit:
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp) unless explicitly coordinated
- [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)
- [`todo.md`](/workspaces/c4c/todo.md), [`todoB.md`](/workspaces/c4c/todoB.md), [`todoC.md`](/workspaces/c4c/todoC.md), [`todoD.md`](/workspaces/c4c/todoD.md)

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/x86/codegen/emit.cpp.o`

Handoff standard:
- report the remaining x86 `BackendModule` helper clusters
- report which clusters already have enough BIR shape
- report which clusters still need new BIR fields or shared helper contracts

Status:
- ready for BIR parity wave
