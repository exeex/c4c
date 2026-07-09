Status: Active
Source Idea Path: ideas/open/637_prepared_stack_destination_fan_in_authority_producer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Legal And Negative Fan-In Behavior

# Current Packet

## Just Finished

Step 3 proved the selected semantic-merge producer contract without code
changes. The focused legal shape is the select-materialized stack-destination
fan-in with a preserved stack fallback:
`PreparedMoveAuthorityKind::StackDestinationRegisterFanIn` plus
`PreparedStackDestinationFanInSemantics::SelectMaterializationPreservedStackFallback`.
Focused tests prove that this shape publishes/exposes
`stack_destination_fan_in_authority` with owner
`prepared_stack_destination_register_fan_in` and moves past the
missing-authority owner; the RV64 object-emission coverage also verifies the
authorized fixture emits both register-source stores.

Negative behavior remains fail-closed. Coverage verifies that a select-shaped
fan-in before publication still reports `authority=none` with
`missing_stack_destination_fan_in_authority_fact`; bundle-only or malformed
authority rejects as
`mismatched_stack_destination_register_fan_in_move_authority`; unsupported,
unknown, generic ambiguous, and non-select fan-in authority shapes do not
publish or consume the selected fact.

The six spillover rows remain outside the selected
semantic-merge/select-materialized producer contract:

- `src/20011109-2.c`: still `authority=none`, `move_count=3`, and
  `fragment_status=missing_stack_destination_fan_in_authority_fact`.
- `src/20021204-1.c`, `src/920429-1.c`, `src/930429-1.c`,
  `src/pr34415.c`, and `src/ptr-arith-1.c`: still non-parallel
  two-register-source stack-destination fan-ins with `authority=none` and
  `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.

Those six rows do not justify another producer packet under idea 637's selected
contract. They are residuals for a different destination-authority family
(ordered final-state or mutual-exclusion) or a downstream RV64 consumer idea
after a matching producer contract exists.

## Suggested Next

Run Step 4 broader consistency/closure review for the prepared/prealloc
authority surface. If that stays green, recommend lifecycle review for closure
or downstream follow-up rather than widening idea 637 into another producer
family.

## Watchouts

The supplemental row probe still reports `total=6 passed=0 failed=6`, which is
expected for this validation packet and should not be reclassified as RV64
materialization progress. Keep source freshness separate from destination
fan-in authority. Do not infer authority from filenames, block labels, value
ids, source order, move-vector order, diagnostics, ABI/runtime, or final
assembly.

## Proof

Canonical command run:

```text
cmake --build --preset default --target backend_prepare_stack_layout_test -j1 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1
```

Result: passed. Root proof log: `test_after.log`.

Supplemental focused validation:

```text
cmake --build --preset default --target backend_riscv_object_emission_test -j1 && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > build/agent_state/637_step3_riscv_object_emission.log 2>&1
```

Result: passed. Log:
`build/agent_state/637_step3_riscv_object_emission.log`.

Supplemental row probe:

```text
cmake --build --preset default && ALLOWLIST=build/agent_state/637_step1_destination_fan_in.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/637_step3_destination_fan_in.log 2>&1
```

Result: nonzero as expected for fail-closed spillover evidence,
`total=6 passed=0 failed=6`. Log:
`build/agent_state/637_step3_destination_fan_in.log`.
