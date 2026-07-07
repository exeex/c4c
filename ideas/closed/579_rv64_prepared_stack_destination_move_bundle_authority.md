# RV64 Prepared Stack-Destination Move-Bundle Authority

Status: Closed
Type: Capability repair
Parent: `ideas/closed/574_rv64_floating_point_binary_lowering.md`
Owning Layer: RV64 prepared object-route move-bundle classification and
materialization authority

## Goal

Diagnose and repair the RV64 object-route blocker where the prepared
move-bundle classifier rejects an ambiguous non-parallel multi-source
stack-destination authority after floating-point binary lowering has advanced.

## Why This Exists

The 574 floating-point binary work advanced `src/20000605-1.c` past the
original double and float `BinaryInst` owners. The next representative blocker
is no longer an FP binary unsupported owner. It is a prepared consumer
classification failure:

```text
prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination:
prepared move-bundle classifier rejected ambiguous non-parallel multi-source
stack-destination authority
```

Evidence:

- `build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/object-route.log`
- `build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/object-route.rc`
- `build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/prepared-focus.txt`

## In Scope

- Reproduce and localize the first prepared move-bundle classifier rejection
  reached after 574.
- Identify the involved move-bundle authority, source values, destination stack
  slot, instruction owner, and whether the sources are mutually exclusive,
  ordered, or genuinely ambiguous.
- Add focused backend/prepared-object coverage for the valid stack-destination
  authority shape if it is semantically lowerable.
- Repair RV64 prepared object-route classification or materialization for that
  semantic shape.
- Keep genuinely ambiguous non-parallel multi-source stack destinations
  fail-closed with a specific diagnostic.

## Out Of Scope

- Further FP binary lowering, F128 lowering, FP cast/truncation work, or runtime
  comparison repairs.
- Pointer arithmetic, inline asm carrier, call ABI, select, or same-module call
  repairs unless evidence proves they are the first semantic owner behind this
  exact move-bundle authority.
- Filename-specific handling for `src/20000605-1.c`.
- Weakening prepared move-bundle validation so invalid stack authority becomes
  silently accepted.

## Acceptance Criteria

- The first move-bundle classifier rejection is recorded with prepared-BIR or
  route evidence naming the owner, sources, destination, and authority shape.
- Focused tests cover the supported stack-destination move-bundle shape and at
  least one genuinely ambiguous fail-closed shape.
- The representative route no longer fails at the same
  `ambiguous_non_parallel_multi_source_stack_destination` classifier rejection,
  or it fails with a narrower diagnostic proving the authority is invalid.
- Any later FP cast, runtime mismatch, pointer, call, select, or unrelated
  owner is recorded as a separate follow-up instead of being mixed into this
  idea.

## Closure Notes

Closed on the narrower fail-closed acceptance path. Step 1 localized the
prepared move bundle to two register sources, `value_id=22` and `value_id=23`,
fanning into stack-slot `value_id=24` before `render_image_rgb_a` block
`for.cond.2` instruction index 2 with no producer ordering, mutual exclusion,
or parallel-copy authority.

Focused RV64 object-emission coverage now asserts the narrower diagnostic
`producer_authority_missing_for_register_fan_in_stack_destination` for the
two-register-source fan-in to one stack destination, while preserving the
shared `ambiguous_non_parallel_multi_source_stack_destination` category for a
genuinely ambiguous stack-destination bundle.

The representative route still exits with code 2, but it no longer leaves the
owner classified only by the old generic ambiguity. Its current route evidence
is saved under
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step5/src_20000605-1.c/`
and includes:

```text
diagnostic_owner=rv64_prepared_move_bundle_consumer
fragment_status=producer_authority_missing_for_register_fan_in_stack_destination
```

Backend closure proof used:

```bash
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

The close-time regression guard compared matching backend `test_before.log`
and `test_after.log` captures.

## Reviewer Reject Signals

- Reject filename-, function-, or value-name-specific handling for
  `src/20000605-1.c`, `render_image_rgb_a`, or the exact temporary names in the
  574 artifact.
- Reject disabling or broadly bypassing the ambiguous move-bundle classifier as
  the claimed repair.
- Reject expectation rewrites, unsupported-marker edits, allowlist changes, or
  route classification edits claimed as capability progress.
- Reject broad prepared-move rewrites that do not prove the specific
  stack-destination authority shape and preserve fail-closed behavior for
  genuinely ambiguous multi-source cases.
- Reject mixing further FP binary, cast, runtime mismatch, pointer, call,
  inline asm, or select repairs into this completion claim.
