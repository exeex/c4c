# x86 Prepared Edge Publication Remaining Home Coverage

## Goal

Broaden the remaining nearby x86 prepared edge-publication source and
destination home combinations before treating the shared prepared
edge-publication contract as ready for a separate RISC-V consumer idea.

## Why This Exists

Idea 22 broadened x86 shared edge-publication consumption from the initial
idea 21 stack-source to register-destination path to register-source to
register-destination movement. The final handoff explicitly left more x86
edge/home combinations to cover before RISC-V readiness. The next slice should
keep extending the same x86 consumer family without changing authority
boundaries or claiming readiness from a partially covered x86 surface.

Shared prepare remains the semantic authority for edge-publication facts and
lookup indexing. x86 remains responsible for scratch selection, clobber
policy, physical register spelling, register-class constraints, stack operand
syntax, move spelling, branch/control-flow emission, and final assembly
formatting.

## In Scope

- Extend x86 prepared edge-publication consumption to one or more remaining
  nearby source/destination home combinations, especially destination homes
  that were intentionally left unsupported by idea 22.
- Continue using
  `x86::ConsumedPlans::shared_function_lookups()->edge_publications` as the
  only semantic source of edge-publication facts.
- Keep unsupported homes explicit when the current x86 lowering surface still
  cannot emit them safely.
- Add focused tests that distinguish real shared lookup consumption from
  x86-local rediscovery or named-case matching.
- Preserve existing AArch64 behavior and shared prepared lookup contracts.
- Produce a final handoff stating whether x86 coverage is now broad enough for
  a separate RISC-V consumer idea or whether another x86 coverage slice remains
  necessary.

## Out Of Scope

- RISC-V consumer implementation or readiness claims before the final handoff.
- Full x86 codegen completion.
- Broad x86 control-flow lowering rewrites.
- Creating an x86-local edge-copy fact table, predecessor/successor scanning
  path, or fallback semantic authority.
- Moving x86 register names, scratch policy, clobber sequencing, stack syntax,
  move spelling, or assembly formatting into shared prepare, BIR, or
  target-neutral helpers.
- Weakening supported-path expectations, marking newly targeted paths
  unsupported as a substitute for capability work, or accepting
  expectation-only progress.

## Acceptance Criteria

- At least one additional remaining x86 edge-publication home combination or
  nearby edge shape consumes shared prepared edge-publication facts through the
  existing shared lookup authority.
- The selected coverage is not only the idea 21 stack-source to
  register-destination path or the idea 22 register-source to
  register-destination path.
- Focused tests fail if x86 ignores shared `edge_publications`, uses a local
  edge-copy source of truth, or matches only a named testcase shape.
- Missing-authority and still-unsupported-home behavior remains explicit and
  fails closed.
- Validation covers the new x86 coverage, the relevant shared prepared lookup
  surface, and a backend bucket appropriate for the touched lowering surface.
- The final handoff makes a concrete RISC-V-readiness decision based on the
  remaining x86 edge/home coverage, not on aspiration.

## Reviewer Reject Signals

- A patch matches only one named testcase, fixture label, or edge name instead
  of implementing a semantic x86 home or edge-family rule.
- A patch adds or extends x86-local edge-copy facts, predecessor/successor
  rediscovery, or fallback publication authority instead of reading shared
  `edge_publications`.
- A patch downgrades supported-path expectations, marks the targeted remaining
  home unsupported, or weakens tests to claim progress.
- A patch claims RISC-V readiness while stack-slot destinations or other
  explicitly selected remaining x86 homes still retain the old unsupported
  behavior without a documented fail-closed rationale.
- A patch moves x86 emission policy into shared prepare, BIR, or
  target-neutral helpers.
- A patch broadens into unrelated backend rewrites instead of landing auditable
  x86 prepared edge-publication coverage.

## Closure Notes

Closed on 2026-05-26.

Idea 23 is complete. The active runbook landed an additional remaining x86
home family by extending shared `edge_publications` consumption to
`RematerializableImmediate -> Register` moves when the source home publishes
`immediate_i32`. Together with the earlier x86 slices, the shared publication
consumer surface now covers `StackSlot -> Register`, `Register -> Register`,
and `RematerializableImmediate -> Register`.

The final handoff decided that a separate RISC-V consumer idea is justified
now for register-destination edge-publication moves only. This is not a
full-home readiness claim: `PointerBasePlusOffset -> Register` and
source-to-`StackSlot` destinations remain explicitly unsupported and
fail-closed on x86.

Close evidence:

- `review/idea23_x86_immediate_edge_publication_review.md` reported no
  blocking findings and recommended proceeding to the Step 5 handoff.
- `test_before.log` contains the accepted post-commit backend bucket evidence:
  162/162 backend tests passed.
- `test_baseline.log` contains accepted full-suite evidence: 3410/3410 tests
  passed.
- Close-time regression guard parsed the accepted backend evidence with
  `test_before.log` as both before and after for this lifecycle-only closure:
  162/162 passed, no new failures.
