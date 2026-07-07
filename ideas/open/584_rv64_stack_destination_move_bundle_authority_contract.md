# RV64 Stack-Destination Move-Bundle Authority Contract

Status: Open
Type: Prepared/RV64 architecture repair
Parent: `ideas/closed/579_rv64_prepared_stack_destination_move_bundle_authority.md`
Owning Layer: Prepared move-bundle authority and RV64 prepared-object consumer

## Goal

Define and implement a narrow authority contract for prepared move bundles that
fan multiple register sources into one stack destination, so the RV64 consumer
accepts only authority-backed fan-in and keeps genuinely ambiguous shapes
fail-closed.

## Why This Exists

Idea 579 closed on a narrower fail-closed path after localizing the
`src/20000605-1.c` blocker to two register sources, `value_id=22` and
`value_id=23`, fanning into stack-slot `value_id=24` before
`render_image_rgb_a` block `for.cond.2` instruction index 2. The route had no
producer ordering, mutual exclusion, or parallel-copy authority visible to the
prepared consumer.

The close-time diagnostic is now more precise:

```text
diagnostic_owner=rv64_prepared_move_bundle_consumer
fragment_status=producer_authority_missing_for_register_fan_in_stack_destination
```

That diagnostic is useful, but it is not the architecture fix. The remaining
gap is the missing contract that says when multi-source fan-in to one stack
destination is semantically legal and what the RV64 consumer is allowed to
emit. The consumer must not guess legality from source order, value ids,
function names, or target-shaped fragments.

## In Scope

- Define a small move-bundle authority taxonomy for stack-destination
  multi-source fan-in.
- At minimum, represent `unknown` or missing authority and one supported legal
  authority such as `equivalent_carrier`, `exclusive_control_flow`,
  `ordered_copy`, or `parallel_copy`.
- Record stack-destination fan-in facts in focused diagnostics or prepared
  object evidence:
  - destination stack slot / value id
  - source value ids and homes
  - producer owner
  - authority kind
  - missing-authority reason when authority is absent
- Teach the RV64 prepared-object consumer to accept only supported
  authority-backed stack-destination fan-in.
- Add focused coverage for one supported legal fan-in shape and one genuinely
  ambiguous or missing-authority shape.
- Re-run the 579 representative route and record whether it advances past the
  current blocker or remains blocked by a named missing producer authority.

## Out Of Scope

- Broad prepared move-bundle rewrites that do not first define the authority
  contract consumed by RV64.
- Weakening or bypassing the ambiguous move-bundle classifier.
- Reconstructing producer authority in RV64 from CFG guesses, source order,
  value ids, filenames, block names, or emitted instruction offsets.
- Select, call, pointer arithmetic, floating-point, variadic helper, F128, or
  runtime comparison repairs unless focused evidence proves they are the first
  owner after this authority contract is satisfied.
- Rewriting expectations, unsupported markers, allowlists, route
  classification, or diagnostics as a substitute for capability repair.

## Acceptance Criteria

- The implementation defines a concrete authority taxonomy for stack
  destination fan-in, including missing/unknown authority and at least one
  supported legal authority.
- Focused tests prove that a legal multi-source-to-one-stack-destination bundle
  with explicit authority is accepted and materialized correctly.
- Focused tests prove that missing or unknown authority remains fail-closed
  with a specific diagnostic, not a broad classifier bypass.
- RV64 prepared-object lowering consumes prepared authority facts; it does not
  infer fan-in legality from testcase names, source order, value ids, CFG
  reconstruction, or diagnostic strings.
- The `src/20000605-1.c` 579 representative is rerun:
  - if the fan-in is legal, it advances past
    `producer_authority_missing_for_register_fan_in_stack_destination`
  - if the fan-in is not yet legal, the route records the exact producer
    authority still missing and names the upstream owner
- Existing genuinely ambiguous stack-destination bundles continue to
  fail-closed.
- Backend validation for the touched prepared/RV64 object-emission bucket
  passes with matching before/after proof.

## Reviewer Reject Signals

- Reject filename-, function-, block-, value-id-, or offset-specific handling
  for `src/20000605-1.c`, `render_image_rgb_a`, `for.cond.2`,
  `value_id=22`, `value_id=23`, or `value_id=24`.
- Reject any route that accepts multi-source stack-destination fan-in without a
  visible authority kind and owner facts.
- Reject disabling, weakening, or broadly bypassing the ambiguous
  move-bundle classifier as the claimed repair.
- Reject RV64-side CFG reconstruction, source-order guessing, last-writer-wins
  behavior, or diagnostic-string matching as authority.
- Reject a diagnostic-only change that leaves every legal stack-destination
  fan-in unsupported.
- Reject broad prepared move, select, call, pointer, FP, or runtime rewrites
  that do not prove the stack-destination authority contract.
- Reject expectation rewrites, unsupported-marker edits, allowlist changes, or
  weaker test contracts without explicit user approval.
