Status: Active
Source Idea Path: ideas/open/637_prepared_stack_destination_fan_in_authority_producer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Prepared Destination Authority Facts

# Current Packet

## Just Finished

Step 2 verified the prepared/prealloc publication path for the selected
semantic-merge authority family:
`PreparedMoveAuthorityKind::StackDestinationRegisterFanIn` plus
`PreparedStackDestinationFanInSemantics::SelectMaterializationPreservedStackFallback`.

The current code publishes authority only for a select-materialized
stack-destination fan-in bundle whose select result has a prepared
select-materialization source producer, whose destination home is a stack slot,
and whose participating moves include at least two register sources plus a
preserved stack fallback. The normal prealloc publish pipeline calls
`populate_stack_destination_register_fan_in_move_authority`, which marks the
bundle and every participating move as
`StackDestinationRegisterFanIn`; the prepared object consumer then exposes the
`stack_destination_fan_in_authority` fact with
`select_materialization_preserved_stack_fallback` semantics when bundle and move
facts agree.

Negative focused coverage stayed fail-closed: a select-shaped fan-in before
publication still reports `authority=none` with
`missing_stack_destination_fan_in_authority_fact`, bundle-only or malformed
authority rejects as
`mismatched_stack_destination_register_fan_in_move_authority`, and unsupported
or non-select authority shapes do not publish the selected fact.

## Suggested Next

Start Step 3 by proving the legal and negative fan-in behavior from the
producer side. Use the focused publication/fail-closed assertions plus the six
idea-630 spillover rows to show the route moved only the legal
select-materialized shape through the missing-authority owner and left
non-selected register fan-ins fail-closed for a later authority family or RV64
consumer idea.

## Watchouts

The supplemental row probe still reports `total=6 passed=0 failed=6`, which is
expected for this packet because Step 2 did not add RV64 materialization and the
six rows are still not authorized by the selected select-materialization
producer. Current first owners remain:

- `src/20011109-2.c`: `authority=none`,
  `fragment_status=missing_stack_destination_fan_in_authority_fact`.
- `src/20021204-1.c`, `src/920429-1.c`, `src/930429-1.c`,
  `src/pr34415.c`, and `src/ptr-arith-1.c`: two-register-source
  stack-destination fan-in with `authority=none` and
  `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.

Keep source freshness separate from destination fan-in authority. Do not infer
authority from filenames, block labels, value ids, source order, move-vector
order, diagnostics, ABI/runtime, or final assembly.

## Proof

Canonical command run:

```text
cmake --build --preset default --target backend_prepare_stack_layout_test -j1 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1
```

Result: passed. Root proof log: `test_after.log`.

Supplemental focused validation:

```text
cmake --build --preset default --target backend_riscv_object_emission_test -j1 && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'
```

Result: passed.

Supplemental row probe:

```text
cmake --build --preset default && ALLOWLIST=build/agent_state/637_step1_destination_fan_in.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/637_step2_destination_fan_in.log 2>&1
```

Result: nonzero as expected for fail-closed spillover evidence,
`total=6 passed=0 failed=6`. Log:
`build/agent_state/637_step2_destination_fan_in.log`.
