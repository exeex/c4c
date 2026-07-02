Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Triage The 17 Residual Register-To-Stack Rows

# Current Packet

## Just Finished

Executed Step 7 for the 17 residual register-to-stack rows.

Implemented one general RV64/MIR materialization rule in
`src/backend/mir/riscv/codegen/object_emission.cpp`: coherent
rematerializable integer immediate sources can now use the existing RV64
load-immediate helper before storing to an authorized stack-slot destination,
instead of being limited to 12-bit immediates.

Recorded Step 7 row accounting in
`docs/rv64_gcc_torture_post_contract/move_bundle_materialization_residual_register_to_stack.md`.
Derived proof artifacts live under
`build/agent_state/551_step7_register_to_stack_residual/`.

Fresh Step 7 counts:

- 17 rows scanned.
- 2 rows now pass: `src/bf-pack-1.c`, `src/pr25125.c`.
- 6 rows reroute to prepared move-bundle classifier ownership with
  `ambiguous_non_parallel_multi_source_stack_destination`.
- 6 rows still report generic move-bundle materialization failure, all with
  row-level reroute evidence in
  `build/agent_state/551_step7_register_to_stack_residual/rerouted_generic_failures.tsv`.
- 2 rows advanced to runtime mismatch.
- 1 row advanced to `unsupported_terminator_fragment`.

No source ideas, expectation files, unsupported markers, allowlists outside
`build/agent_state`, or runtime comparison code were changed.

## Suggested Next

Delegate Step 8 for the three residual
`rematerializable_immediate_to_stack_slot` rows: `src/920721-1.c`,
`src/pr82192.c`, and `src/usmul.c`.

## Watchouts

- Do not repair the six Step 7 generic reroutes by inventing homes or offsets
  in RV64. Current evidence points at prepared destination/source authority:
  rematerializable-immediate destination homes, pointer-base-plus-offset
  destination homes, or missing source scalar size authority.
- The six `ambiguous_non_parallel_multi_source_stack_destination` rows are
  prepared classifier or authority work, not RV64 materialization work.
- The Step 7 subset still has later-route residuals:
  `unsupported_terminator_fragment` for `src/pr48197.c` and runtime mismatch
  for `src/20020510-1.c` and `src/pr89195.c`.

## Proof

- Full delegated Step 7 proof output is preserved in `test_after.log`.
- `cmake --build --preset default` passed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed:
  345/345.
- `ALLOWLIST=build/agent_state/551_step7_register_to_stack_residual.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh`
  reported `total=17 passed=2 failed=15`.
- Reconciliation reported
  `generic_move_bundle_materialization_failed=6`,
  `reroute_prepared_move_bundle_classifier=6`,
  `later_runtime_mismatch=2`, `later_explicit_unsupported_diagnostic=1`,
  `pass=2`, and `generic_reroute_rows=6`.
- `git diff --check -- src/backend/mir/riscv/codegen/object_emission.cpp todo.md docs/rv64_gcc_torture_post_contract/move_bundle_materialization_residual_register_to_stack.md test_after.log`
  passed.
