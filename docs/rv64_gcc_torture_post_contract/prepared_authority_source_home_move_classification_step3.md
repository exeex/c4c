# Prepared Authority Step 3 Source-Home And Move-Classification Repair

Status: Step 3 implementation packet for
`ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md`.

## Rule

Prepared value-location publication now normalizes ordinary consumer
stack-destination move reasons from explicit published source-home facts. When
the move is a non-parallel `consumer_*` value move to stack storage and the
published source home is a register, the prepared move-bundle reason is
published as `consumer_register_to_stack`; when the source home is a stack
slot, it remains `consumer_stack_to_stack`.

The rule is grounded in prepared value homes and move metadata. It does not
inspect testcase names, expected assembly, RV64 register spelling, or RV64
diagnostic text. It also leaves non-parallel multi-source stack-destination
cases explicit; those remain classifier-owned reroutes rather than generic
materialization work.

## Focused Rows

The Step 3 allowlist contains the 15 Step 1 queue rows where
`earliest_missing_fact_group=source_home_or_move_classification`:

- `build/agent_state/552_step3_source_home_move_classification.allowlist`
- `build/agent_state/552_step3_source_home_move_classification/row_status.tsv`
- Proof log: `test_after.log`

## Results

Focused proof result:

```text
[rv64-gcc-torture] total=15 passed=3 failed=12
```

Row classifications after the repair:

| Classification | Rows |
| --- | ---: |
| Repaired and passing | 3 |
| Rerouted to earlier producer or explicit multi-source authority | 9 |
| Advanced to later diagnostic | 3 |
| Still source-home or move-classification authority gap | 0 |

Repaired/pass rows:

- `src/20011219-1.c`
- `src/20070212-1.c`
- `src/vrp-6.c`

Rows rerouted to earlier producer or explicit multi-source authority:

- `src/20000113-1.c`
- `src/20020226-1.c`
- `src/20020508-1.c`
- `src/20020508-2.c`
- `src/20020508-3.c`
- `src/20041114-1.c`
- `src/mode-dependent-address.c`
- `src/pr40386.c`
- `src/pr81281.c`

Rows advanced to later diagnostics:

- `src/20070212-3.c`: `unsupported_local_memory_access`
- `src/991216-1.c`: select publication `unsupported_source_stack_offset`
- `src/pr35800.c`: `unsupported_local_memory_access`

## Proof

```text
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
ALLOWLIST=build/agent_state/552_step3_source_home_move_classification.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Results:

- Build passed.
- Backend CTest passed `345/345`.
- Focused allowlist passed `3/15`; no row remains blocked by the original
  source-home or consumer move-class mismatch.

Proof output is preserved in `test_after.log`.
