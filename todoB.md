Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: B

Ownership:
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/aarch64/codegen/emit.hpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.hpp) if needed

Goal:
- prepare aarch64 to consume the first new BIR contract additions, focusing on
  the known call / extern / global / string / local-slot gaps

Priority migration map:
- turn the current aarch64 `BackendModule` clusters into a concrete switch
  order
- identify the first helpers that should move once bounded BIR shape lands
- make only small clarifying edits that reduce migration friction

Do:
- keep behavior unchanged
- prefer comments, helper regrouping, or tiny signature-local clarifications
- record any dependency on Group C when the missing piece is really shared BIR
  contract

Do not edit:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp) unless explicitly coordinated
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)
- [`todo.md`](/workspaces/c4c/todo.md), [`todoA.md`](/workspaces/c4c/todoA.md), [`todoC.md`](/workspaces/c4c/todoC.md), [`todoD.md`](/workspaces/c4c/todoD.md)

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/aarch64/codegen/emit.cpp.o`

Handoff standard:
- report the first aarch64 helpers that can switch after bounded BIR additions
- report which exact BIR contract each helper still lacks
- report any clarifying edits landed in this slice

Status:
- ready for BIR contract expansion wave
