# Current Packet

Status: Active
Source Idea Path: ideas/open/746_bir_node_kind_centric_storage_pass_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish the durable BIR phase and storage contract

## Just Finished

- Completed `plan.md` Step 4 as a documentation-only contract publication in
  `src/backend/bir/core/README.md`.
- Published the flat arena-owned node model, value-owned concrete type,
  input-only operands, parameter-only payload role, exact landed NodeKind
  descriptor/helper surfaces, fail-closed runtime queries, and explicit pass
  dispatch obligations.
- Documented the target-independent B1-B7 vocabulary transition with B4 SSA
  timing, distinct later C-F preparation/pseudo/allocation/MIR ownership, and
  immutable exact-revision external analysis/phase products.
- Explained why the single C++ traits/variant authority needs no TableGen or
  `.td` side language; marked vectors/results/result-index as bootstrap
  compatibility; chose single ordinary result plus normalization/projection as
  the preferred durable route; and bounded `ResultSpan`/external result tables
  to separately approved follow-up work.
- Corrected the implementation-state summary to acknowledge the landed
  16-kind schema and verifier helpers without claiming complete A, B, or C-F
  implementation. Closed ideas 735/736 are cited only as historical contracts.

## Suggested Next

- Execute only `plan.md` Step 5 final proof/review: compare the README against
  idea 746 acceptance criteria, run the supervisor-selected fresh build and
  focused/broader tests, and make the explicit lifecycle conclusion decision.

## Watchouts

- The older exhaustive receiving matrices remain historical phase-A target
  inventory and contain stale per-row implementation dispositions. The README
  now labels them accordingly and makes the current Implementation State and
  landed NodeKind section authoritative; Step 5 should reject any reading that
  treats those old dispositions as current coverage.
- The preferred single-result normalization is durable direction, not landed
  storage. Step 5 must preserve the explicit partial-status boundary.

## Proof

- Supervisor-selected proof: `git diff --check > test_after.log 2>&1`.
- Result: passed; log path `test_after.log`.
- This is sufficient for the documentation-only Step 4 packet; Step 5 owns the
  fresh shared-code build and focused/broader regression proof.
