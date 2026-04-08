Status: Parked
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: D

Ownership:
- [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
- [`tests/backend/backend_lir_adapter_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_aarch64_tests.cpp)
- [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
- directly related backend test support files only if needed for migrated
  assertions

Goal:
- no active work in this round; the legacy adapter test family is already gone

Priority kill list:
- delete files that are now pure legacy scaffolding
- migrate any still-valuable assertions into surviving BIR/native test files
- avoid reintroducing legacy backend-IR dependencies into active tests

Do:
- keep migrated assertions narrow and architecture-aligned
- coordinate with mainline before touching broadly shared support
- report whether each legacy file was deleted outright or had assertions moved
  elsewhere

Do not edit:
- production emitter files
- shared lowering files unless directly required by an assertion migration and
  explicitly coordinated
- [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [`src/apps/c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)
- [`todo.md`](/workspaces/c4c/todo.md), [`todoA.md`](/workspaces/c4c/todoA.md), [`todoB.md`](/workspaces/c4c/todoB.md), [`todoC.md`](/workspaces/c4c/todoC.md)

Worker-local validation:
- build only the touched representative object, for example:
  `cmake --build build -j8 --target CMakeFiles/backend_bir_tests.dir/tests/backend/backend_bir_pipeline_tests.cpp.o`

Handoff standard:
- report which legacy files were deleted
- report any assertion moved into a surviving test file
- report the smallest remaining reason any `backend_lir_adapter*` file had to
  stay

Status:
- parked after second-wave completion; do not dispatch unless a new bounded
  test-only follow-up appears
