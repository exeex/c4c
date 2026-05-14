Status: Active
Source Idea Path: ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Backend Test Ownership

# Current Packet

## Just Finished

Step 1 inventory completed for idea 220. Current backend surface:

- CMake wiring: `tests/backend/CMakeLists.txt` owns one flat
  `BACKEND_TEST_ROOT`, one shared `BACKEND_CASE_ROOT`, helper functions
  `c4c_collect_backend_labels`, `c4c_set_backend_test_labels`,
  `c4c_disable_backend_test_when_x86_tests_off`,
  `c4c_add_backend_codegen_route_test`, and
  `c4c_add_backend_runtime_case`; all current backend labels include
  `backend`, with more specific labels such as `internal`, `cpp`, `cli`,
  `aarch64`, `x86`, `backend_route`, `backend_runtime`, `case`, and
  `external_toolchain`.
- Current listings: 49 top-level backend `*_test.cpp` files, 169
  `tests/backend/case/*.c` fixtures, 5 helper CMake scripts under
  `tests/backend/cmake`, and 147 configured `backend_` CTest entries.
- `bir-live`: `backend_prepare_*`, `backend_lir_to_bir_notes`,
  `backend_prepared_printer`, `backend_x86_prepared_handoff_label_authority`,
  live `backend_cli_dump_bir*` and `backend_cli_dump_prepared_bir*`, riscv64
  semantic route tests, and x86_64 `backend_codegen_route_*_observe_semantic_bir`
  tests whose required snippets assert BIR/prepared-BIR text.
- `mir-live`: all live `backend_aarch64_*` C++ unit/model/printer/record tests;
  `backend_cli_aarch64_asm_no_machine_nodes_fails`;
  `backend_cli_aarch64_asm_external_return_zero_smoke`; and
  `backend_cli_aarch64_asm_external_return_add_smoke`.
- Confirmed the two AArch64 MIR case files named by the source idea are present
  and live: `tests/backend/case/aarch64_no_selected_machine_nodes.c` is used by
  `backend_cli_aarch64_asm_no_machine_nodes_fails`, and
  `tests/backend/case/aarch64_return_zero_smoke.c` is used by
  `backend_cli_aarch64_asm_external_return_zero_smoke`.
- `shared-fixture`: `tests/backend/case/*.c` is a mixed input corpus, not proof
  ownership. BIR-live examples include `vla_goto_stackrestore.c`,
  `aggregate_param_return_pair.c`, `local_arg_call.c`, `three_way_phi_merge_post_add_sub.c`,
  and the x86_64 semantic-route fixture family. `return_add.c` is a shared
  general fixture currently used by the AArch64 external return-add smoke test,
  so the test is MIR-owned while the fixture should not become an AArch64 case
  owner.
- `legacy`: optional `C4C_ENABLE_X86_BACKEND_TESTS` C++ handoff/route-debug
  tests are flat-tree x86 backend-era tests. They are not MIR proof; audit them
  as BIR/prepared-BIR or legacy before any later move.
- `deprecated` / `delete`: 12 disabled `backend_cli_dump_mir_*` and
  `backend_cli_trace_mir_*` placeholders are disabled through
  `c4c_disable_backend_test_when_x86_tests_off` and assert x86 debug-shell text
  such as route owner/module emitter/deferred diagnostics rather than real
  target MIR records. They should be deleted unless a later packet first
  converts one into an active MIR-owned contract.

## Suggested Next

First mechanical move packet: create `tests/backend/bir/` and mechanically move
the obvious BIR C++ unit tests only: `backend_prepare_*_test.cpp`,
`backend_lir_to_bir_notes_test.cpp`, `backend_prepared_printer_test.cpp`, and
`backend_x86_prepared_handoff_label_authority_test.cpp`. Update only the
corresponding source paths in `tests/backend/CMakeLists.txt`; do not move CLI
dump tests, route tests, fixtures, AArch64 tests, or disabled MIR placeholders in
that packet.

Suggested narrow proof command for that packet:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^(backend_prepare_|backend_lir_to_bir_notes|backend_prepared_printer|backend_x86_prepared_handoff_label_authority)" --output-on-failure' > test_after.log 2>&1`

## Watchouts

- The current label set cannot independently select BIR vs MIR yet; all
  backend tests share broad `backend` plus ad hoc `cpp`/`cli`/`aarch64`/route
  labels.
- Disabled MIR dump/trace placeholders are still configured and visible in
  `ctest -N`; deletion should wait for the pruning packet so the reason is
  recorded with the removal.
- Do not classify `tests/backend/case` files as live proof merely because they
  exist; classify by CMake references and assertion target.
- Do not move `return_add.c` into an AArch64-owned case bucket; it is a shared
  fixture used by one MIR-owned smoke test.

## Proof

Delegated proof command:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -N -R "^backend_"' > test_after.log 2>&1`

Result: passed. The build completed and `ctest -N -R "^backend_"` listed 147
configured backend tests, including the disabled MIR dump/trace placeholders.
Proof log path is `test_after.log`.
