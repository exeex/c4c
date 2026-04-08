Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md
Group: D

Ownership:
- [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
- [`tests/backend/backend_lir_adapter_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_aarch64_tests.cpp)
- [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
- [`tests/backend/backend_x86_64_extracted_tests.cpp`](/workspaces/c4c/tests/backend/backend_x86_64_extracted_tests.cpp)
- [`tests/backend/backend_module_tests.cpp`](/workspaces/c4c/tests/backend/backend_module_tests.cpp)
- directly related backend test support files only if required

Goal:
- delete or migrate the parked legacy backend-IR-centric tests now that they are
  out of the active build graph

Do:
- move any still-valuable assertions into surviving BIR/native test families
- delete pure scaffolding when no longer needed
- coordinate with mainline before deleting broadly shared support

Do not edit:
- production emitter files
- shared lowering files unless needed for a migrated assertion and explicitly coordinated
- [`todo.md`](/workspaces/c4c/todo.md), [`todoA.md`](/workspaces/c4c/todoA.md), [`todoB.md`](/workspaces/c4c/todoB.md), [`todoC.md`](/workspaces/c4c/todoC.md)

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/backend_bir_tests.dir/tests/backend/backend_bir_pipeline_tests.cpp.o`
- if touching another test source, build only that source's matching object target

Status:
- pending implementation
