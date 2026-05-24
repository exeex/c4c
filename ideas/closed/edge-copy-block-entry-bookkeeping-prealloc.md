# Edge Copy and Block Entry Bookkeeping Helpers

## Intent

Move generic Prepared edge-copy and block-entry publication bookkeeping out of
AArch64 dispatch and into prealloc or shared MIR helpers.

## Why This Exists

The current AArch64 backend owns bookkeeping for edge copies and block-entry
publications that comes from prepared move bundles and SSA-edge lowering, not
from AArch64 instruction selection. Concrete parallel-copy emission can remain
target-local, but classification and bookkeeping should not be duplicated by
x86 and later RISC-V.

## Current AArch64-Owned Responsibility

Current responsibility is in `dispatch_edge_copies.cpp` and adjacent block-entry
publication bookkeeping that consumes prepared move bundles and establishes
machine-value state at block boundaries.

## Proposed Destination Layer

`src/backend/prealloc` should own move-bundle consumption helpers when the data
is still a Prepared fact. Shared MIR utilities may own target-independent
machine block-entry bookkeeping once values have become machine records.

The first focused run should choose one of those destinations for the narrow
bookkeeping slice it extracts.

## x86/RISC-V Benefit

x86 prepared block lowering can reuse edge-copy and block-entry bookkeeping
instead of duplicating AArch64's move-bundle consumption. RISC-V benefits later
from a shared SSA-edge publication boundary once its backend consumes Prepared
move bundles.

## In Scope

- Identify the smallest target-neutral edge-copy or block-entry bookkeeping
  unit now owned by AArch64.
- Move that unit to prealloc or shared MIR with a clear ownership rationale.
- Keep concrete target move emission local to AArch64.
- Document how x86 prepared block lowering can consume the helper.
- Prove unchanged AArch64 behavior over multiple control-flow shapes with edge
  moves or block-entry publications.

## Out of Scope

- Rewriting the complete out-of-SSA pipeline.
- Moving target parallel-copy instruction selection or register constraints.
- Combining edge-copy bookkeeping with call-boundary, formal-entry, or operand
  decoding migrations.
- Weakening branch/control-flow tests.

## Proof Needed To Avoid Testcase Overfit

Acceptance needs proof over at least two nearby control-flow shapes, such as a
simple branch/merge and a case with multiple incoming edge publications where
currently supported. The proof must show unchanged AArch64 lowering and must
not only prove the originally noticed testcase.

## Reviewer Reject Signals

Reject if the diff moves target move emission or register constraints into a
generic helper, adds matching for one block label or branch shape, weakens
unsupported expectations, or leaves duplicate AArch64-only bookkeeping active
after claiming shared ownership.

Reject if the chosen destination is unclear between prealloc and shared MIR and
the code crosses both layers without a small explicit contract.

## Completion Note

Closed after the active runbook moved a target-neutral block-entry publication
helper into prealloc-owned value-location support, adapted AArch64 block-entry
publication consumers to use the shared helper while retaining concrete target
move emission locally, and exposed a concrete x86 prepared-query reuse path.

Validation covered the prealloc block-entry publication helper, AArch64
consumer paths, and x86 prepared query reuse through backend tests including
`backend_prealloc_block_entry_publications`,
`backend_cli_dump_prepared_bir_focus_block_entry_00204`,
`backend_aarch64_branch_control_lowering`, and
`backend_x86_prepared_decoded_home_storage`. No tests or expectations were
weakened or reclassified.
