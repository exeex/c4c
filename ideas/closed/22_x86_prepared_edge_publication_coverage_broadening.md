# x86 Prepared Edge Publication Coverage Broadening

## Goal

Broaden x86 prepared edge-publication lowering coverage beyond the first
joined-branch consumer landed by idea 21.

## Why This Exists

Idea 21 proved that x86 can consume shared prepared edge-publication facts from
`x86::ConsumedPlans::shared_function_lookups()->edge_publications` and emit a
target-owned move for one narrow lowering route. The next useful x86 slice is
not RISC-V readiness yet; it is increasing confidence that nearby x86
edge-publication and home combinations use the same shared authority without
reintroducing target-local semantic edge-copy discovery.

The new work should preserve the boundary established by idea 21: shared
prepare owns semantic publication facts and lookup indexing, while x86 owns
scratch selection, clobber policy, physical register spelling, register-class
constraints, stack operand syntax, move spelling, branch/control-flow emission,
and final assembly formatting.

## In Scope

- Extend x86 prepared edge-publication consumption to additional nearby
  edge-publication or block-entry paths.
- Cover additional source and destination home combinations that the current
  x86 lowering surface can support without a broad backend rewrite.
- Reuse the shared prepared lookup authority from
  `x86::ConsumedPlans::shared_function_lookups()->edge_publications`.
- Keep x86 target-local emission details in x86-owned lowering or prepared
  helper code.
- Add focused tests proving the broadened paths fail if shared
  `edge_publications` are absent, ignored, or replaced by local rediscovery.
- Preserve AArch64 behavior and shared prepared lookup contracts.

## Out Of Scope

- Full x86 codegen completion.
- RISC-V consumer implementation or readiness claims.
- Rewriting x86 control-flow lowering broadly.
- Creating an x86-local edge-copy fact table or rediscovery pass.
- Moving x86 emission details into shared prepare, BIR, or target-neutral
  helpers.
- Weakening supported-path expectations, marking covered paths unsupported, or
  accepting expectation-only progress.

## Acceptance Criteria

- More than one x86 edge-publication or block-entry lowering path consumes
  shared prepared edge-publication facts through the existing shared lookup
  authority.
- The broadened coverage includes at least one home or edge shape not covered
  by the initial idea 21 joined-branch consumer proof.
- Focused tests fail if the x86 paths ignore shared `edge_publications` or use
  an independent x86 edge-copy source of truth.
- Missing-authority and unsupported-home behavior remains explicit rather than
  falling back to local semantic rediscovery.
- Validation covers the broadened x86 behavior, the relevant shared prepared
  lookup surface, and a backend bucket appropriate for the touched lowering
  surface.
- The final handoff states whether x86 coverage is broad enough to start a
  separate RISC-V consumer idea or whether more x86 edge/home combinations
  remain first.

## Reviewer Reject Signals

- A patch matches only one named testcase shape instead of broadening the x86
  consumer rule across a nearby edge-publication or home family.
- A patch adds an x86-local edge-copy fact table, predecessor/successor
  rediscovery pass, or fallback semantic authority instead of reading shared
  `edge_publications`.
- A patch moves x86 register names, scratch policy, clobber sequencing, stack
  syntax, move spelling, or assembly formatting into shared prepare or BIR.
- A patch downgrades supported-path expectations, marks the targeted path
  unsupported, or weakens tests to make the broader slice pass.
- A patch claims progress through helper renames, expectation rewrites, or
  classification-only changes while only the original idea 21 path consumes
  shared prepared edge-publication facts.
- A patch broadens into unrelated x86/AArch64/RISC-V lowering rewrites instead
  of adding auditable x86 consumer coverage around edge-publication handling.

## Closure Notes

Closed on 2026-05-26 after the active runbook completed Steps 1 through 4.

The completed slice broadened x86 shared edge-publication consumption from the
idea 21 stack-source to register-destination shape to register-source to
register-destination movement. The x86 path continues to consume shared
prepared edge-publication facts through
`x86::ConsumedPlans::shared_function_lookups()->edge_publications`; it does not
scan BIR edges or reconstruct predecessor/successor publication facts locally.

This satisfies the source idea acceptance criteria:

- more than the original idea 21 x86 movement shape consumes the shared lookup
  authority
- the broadened coverage includes a new source/destination home combination
- focused tests cover the shared-authority dependency and missing-authority
  behavior
- unsupported homes remain explicit instead of falling back to local semantic
  rediscovery
- focused validation passed for the broadened x86 behavior and relevant shared
  prepared lookup surface
- the final handoff states that more x86 edge/home combinations remain before
  starting a separate RISC-V consumer idea

The route review in
`review/idea22_x86_edge_publication_broadening_review.md` found no overfit or
drift. The focused proof was regenerated after review, accepted by the
regression guard, and rolled forward to canonical `test_before.log`.

Close-time regression guard accepted the rolled-forward focused proof. The
delegated close did not regenerate root-level logs because the accepted proof
had already been rolled forward and this lifecycle packet was instructed not
to touch root-level logs. The guard was checked in non-decreasing mode against
that canonical proof:

```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Result: passed 79/79 focused tests, no new failures, non-decreasing pass
count.

Follow-up source idea created:
`ideas/open/23_x86_prepared_edge_publication_remaining_home_coverage.md`.
