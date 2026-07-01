Status: Active
Source Idea Path: ideas/open/510_rv64_selected_object_data_emission.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate And Handoff

# Current Packet

## Just Finished

Step 6 of `plan.md` validated the selected object-data consumer slice for
handoff and recorded the remaining producer-gap boundary.

- Build and focused backend validation passed for RV64 object emission and the
  prepared-object-data ordinary-C coverage.
- Broad backend validation passed:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` ran 339 tests
  with zero failures and wrote `test_after.log`.
- Representative `tests/c/external/gcc_torture/src/20000412-1.c` remains
  marker-only fail-closed with
  `unsupported_global_data: prepared selected object-data contract status=unsupported_but_coherent object_label_id=2 object_size_bytes=1656 emitted_byte_count=0 zero_fill_byte_count=0`;
  this is acceptable for this idea because its prepared facts still lack byte,
  zero-fill, and relocation authority.
- Producer-missing `tests/c/external/gcc_torture/src/930513-2.c` remains
  fail-closed with
  `unsupported_global_data: prepared selected object-data contract status=missing_object_label object_size_bytes=0 emitted_byte_count=0 zero_fill_byte_count=0`.
- Other producer-missing object-label coverage remains fail-closed through
  `backend_cli_riscv64_prepared_object_data_missing_static_label_obj`, which
  passed as an expected-failure object-route case.
- No expectation, unsupported-marker, allowlist, runtime-comparison, scan
  accounting, or default-contract downgrade was used as progress. The changed
  test/build files only add focused positive coverage and one retained
  fail-closed missing-object-label case.

## Suggested Next

Plan owner should close or retire this runbook as the RV64 selected
object-data consumer slice is validated. Narrow follow-up should stay limited
to producer publication for missing static-local object labels, prepared
byte/zero-fill facts, and relocation-ready selected object-data records before
any additional RV64 consumption work.

## Watchouts

- Representative `20000412-1.c` is not a regression in this slice: it is
  marker-only `unsupported_but_coherent` and still lacks prepared bytes,
  zero-fill, and relocation authority.
- Static-local object-data publication, relocation-ready record publication,
  thread-local storage, GOT-required globals, F128 data, broad global access
  widths, broad ABI work, and parameter-home publication remain non-goals for
  this runbook.
- Do not repair producer-missing rows in RV64 by reconstructing storage from
  raw names, C source shape, or known gcc_torture identities.

## Proof

- `scripts/plan_review_state.py set-step --step-id 6 --step-title 'Validate And Handoff'`
  passed.
- `cmake --build build --target c4cll` passed.
- Focused object-emission CTest:
  `ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$'`
  passed.
- Focused prepared-object-data CTest:
  `ctest --test-dir build --output-on-failure -R '^backend_(cli_riscv64_prepared_object_data_|codegen_route_riscv64_prepared_object_data_|obj_runtime_rv64_prepared_object_data_)'`
  passed.
- Focused representative probe:
  `build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000412-1.c -o build/agent_state/510_step6_validate_and_handoff/20000412-1/20000412-1.o`
  failed closed with the expected marker-only selected object-data diagnostic;
  diagnostic captured in
  `build/agent_state/510_step6_validate_and_handoff/20000412-1/probe.out`.
- Focused producer-missing probe:
  `build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/930513-2.c -o build/agent_state/510_step6_validate_and_handoff/930513-2/930513-2.o`
  failed closed with the expected `missing_object_label` diagnostic;
  diagnostic captured in
  `build/agent_state/510_step6_validate_and_handoff/930513-2/probe.out`.
- Downgrade check over changed Step 3-5 files found no allowlist,
  runtime-comparison, scan-accounting, unsupported-marker, or expectation
  downgrade; only focused positive coverage and the retained fail-closed
  missing-object-label expected-failure case were added.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 339/339
  and wrote canonical proof log `test_after.log`.
- `git diff --check -- todo.md`
  passed.
