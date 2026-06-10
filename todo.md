Status: Active
Source Idea Path: ideas/open/164_bir_call_use_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broaden Validation And Prepare Closure

# Current Packet

## Just Finished

Completed Step 5 for
`ideas/open/164_bir_call_use_source_annotation_schema.md`.
Ran broader backend acceptance validation for the Route 6 call-use source
schema migration and recorded closure readiness.

- Route 6 BIR call-use records and indexes are BIR-owned semantic schema over
  `CallInst` and value payloads.
- `bir::find_call_argument_source_producer_materialization(...)` is migrated to
  Route 6 records while keeping the raw same-block scan internal to record
  construction.
- No dedicated public MIR call-use query existed, so no broad MIR,
  target/codegen, ABI assignment, call lowering, aggregate transport, or
  prealloc production consumers were switched.
- Prepared call-use reads remain comparison oracles.
- ABI/layout policy, outgoing stack, register/storage, byval lane, scratch,
  aggregate transport, and destination home fields were not imported into
  Route 6 payloads.
- No expectation downgrades, ABI/layout policy leaks, whole prepared call-plan
  copies, target/codegen rewrites, or helper-only reshuffles were accepted.

## Suggested Next

Ask the plan owner to decide whether idea 164 is ready to close or whether the
source idea needs a follow-up plan.

## Watchouts

- Closure readiness is validation-backed by the broad backend subset, but the
  supervisor/plan-owner still owns lifecycle closure.
- Target/layout-specific call facts remain owned by prepared/prealloc or
  target code, not Route 6 BIR schema.

## Proof

Exact delegated Step 5 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`179/179` matching backend tests passed).
