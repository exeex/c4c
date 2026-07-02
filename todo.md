Status: Active
Source Idea Path: ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Move Classification And Source-Home Authority

# Current Packet

## Just Finished

Step 3, "Repair Move Classification And Source-Home Authority," completed as
a narrow implementation packet. Initial `git status --short` was clean.
Prepared value-location publication now normalizes ordinary consumer
stack-destination move reasons from explicit source-home facts: register
source homes publish `consumer_register_to_stack`, while stack source homes
publish `consumer_stack_to_stack`. Multi-source and non-parallel cases remain
explicit classifier reroutes instead of generic RV64 materialization.

Changed files:

- `src/backend/prealloc/regalloc.cpp`
- `docs/rv64_gcc_torture_post_contract/prepared_authority_source_home_move_classification_step3.md`
- `todo.md`

Derived artifacts:

- `build/agent_state/552_step3_source_home_move_classification.allowlist`
- `build/agent_state/552_step3_source_home_move_classification/row_status.tsv`

Focused Step 3 row counts:

| Classification | Rows |
| --- | ---: |
| Repaired and passing | 3 |
| Rerouted to earlier producer or explicit multi-source authority | 9 |
| Advanced to later diagnostic | 3 |
| Still source-home or move-classification authority gap | 0 |

Repaired rows are `src/20011219-1.c`, `src/20070212-1.c`, and `src/vrp-6.c`.
The explicit reroutes are the non-parallel multi-source stack-destination rows
and conversion-adjacent stack-source stack-destination rows recorded in
`row_status.tsv`. Later diagnostics are `src/20070212-3.c` and
`src/pr35800.c` for local memory access, plus `src/991216-1.c` for select
publication source stack offset.

## Suggested Next

Executor should run Step 4, "Repair Scalar Type, Size, And Alignment
Authority," starting with the 9 scalar-type rows from the Step 1 queue plus
`src/pr36339.c` if the supervisor wants to include the Step 2 carry-forward
row in the scalar packet.

## Watchouts

- Do not weaken the `ambiguous_non_parallel_multi_source_stack_destination`
  guard; those rows need earlier producer or explicit multi-source authority.
- Do not treat conversion-adjacent stack-source stack-destination rows as
  ordinary stack copies; they need producer classification before lowering.
- `src/991216-1.c` has moved out of this packet into select-publication source
  stack-offset evidence.

## Proof

```text
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
ALLOWLIST=build/agent_state/552_step3_source_home_move_classification.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Results:

- Build passed.
- Backend CTest passed `345/345`.
- Focused Step 3 proof passed `3/15`, with `0` rows still blocked by the
  original source-home or consumer move-class mismatch.

Proof output is preserved in `test_after.log`.
