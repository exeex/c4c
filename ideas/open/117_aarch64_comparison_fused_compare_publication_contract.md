# 117 AArch64 Comparison Fused-Compare Publication Contract

## Goal

Make fused-compare and materialized-compare publication facts explicit enough
that `comparison.cpp` consumes shared prepared authority instead of owning
current-block publication lookup and branch-fusion source routing.

## Why This Exists

Idea 115 Step 2 found that `comparison.cpp` is mostly target-local compare and
branch emission, but the current code still has the strongest large-owner
`move-forward` signal outside the dispatch family. The relevant residue is not
the old same-block scan repair from idea 53. Current helper names such as
`find_prepared_materialized_compare_join_targets`,
`find_prepared_fused_compare_operand_producer_facts`,
`collect_prepared_current_block_entry_publications`, and
`prepared_current_block_entry_publication_register` show that comparison
lowering still owns or shapes prepared/current-block publication facts that
should be shared contract inputs before further owner cleanup.

This follow-up exists to define and prove that contract boundary without
turning AArch64 condition-code spelling or branch emission into shared logic.

## In Scope

- `src/backend/mir/aarch64/codegen/comparison.cpp` fused-compare prepared
  operand producer lookup, materialized compare join-target lookup, current
  block-entry publication collection, and prepared publication register lookup.
- Shared prepared/BIR prealloc query owners that can expose branch-condition,
  fused-compare operand, materialized compare, current-block entry publication,
  or join-target facts.
- Dump or route-test visibility that shows the prepared facts consumed by
  fused compare and materialized compare lowering.
- Narrow updates to call sites in AArch64 comparison lowering needed to consume
  a new shared query or a clarified prepared fact.

## Out Of Scope

- Reopening idea 53's old repair around raw same-block cast/load scans,
  constant producer scans, or materialized condition hook matching unless
  current code has regressed to that failure mode.
- Changing condition-code spelling, compare opcode selection, branch emission,
  `cset`/`csel`/`cbnz` fallback shape, register printing, or scratch register
  policy except where a call site must receive prepared facts.
- Folding ALU, dispatch value materialization, or memory publication cleanup
  into this idea.
- Mechanical `comparison.cpp` file splitting or line-count cleanup before the
  shared contract is reviewable.

## Acceptance Criteria

- Fused-compare operand producer facts consumed by `comparison.cpp` come from a
  shared prepared query or clearly owned prepared record, not comparison-local
  routing logic.
- Materialized compare join-target and current-block entry publication facts
  are exposed through shared prepared authority or documented as genuinely
  target-local with dump/test evidence.
- AArch64 comparison lowering remains responsible only for target compare,
  branch, condition-code, and operand emission once prepared facts are known.
- Proof covers fused-compare and materialized/current-block publication paths
  close to the helpers named by idea 115 Step 2.
- Any no-op or keep-local decision is documented with concrete evidence rather
  than inferred from file size.

## Likely Files

- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/dispatch*.cpp` only if needed as consumers
  of the same current-block publication facts
- Shared prepared/BIR prealloc query owners under `src/backend/mir/`
- Existing backend dump or route tests covering fused compare, materialized
  compare conditions, branch conditions, and current-block entry publications

## Proof Route

First characterize the current comparison helper call graph and identify which
facts are target-neutral. Add or tighten dump/test coverage for the prepared
fused-compare/materialized-compare publication facts before removing local
ownership. Then run the supervisor-selected backend/AArch64 subset that covers
comparison, branch fusion, and prepared publication routes.

## Reviewer Reject Signals

- The route reintroduces same-block producer scans, raw terminator recovery,
  BIR-name matching, or named fused-compare shortcuts as permanent authority.
- The old idea 53 failure mode survives under new helper names while the slice
  claims shared contract progress.
- The diff weakens supported expectations, rewrites tests to avoid branch
  fusion, or marks a current supported path unsupported.
- The proof covers only one fused-compare testcase and ignores materialized
  compare or current-block publication neighbors.
- Target-local AArch64 compare/branch spelling is moved into shared prepared
  code.
- The slice expands into broad `comparison.cpp` layout cleanup instead of the
  fused-compare/materialized-publication contract.
