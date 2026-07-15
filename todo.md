# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Implement and prove the Step 8 selection

## Just Finished

814 is capability-complete and archived after its accepted `c1cde8430`
second-shape repair and closed 815's `510388751` mask-lane handoff. The
supervisor accepted the returned fresh build, matching `^backend_` 6/6 guard
with no new failures, representative `scal-to-vec1.c` LLVM emission, and full
CTest 3038/3038. This is prerequisite proof only; no 754 vector row is
selected or complete.

## Suggested Next

Execute Step 9 only. Before any implementation, make and record a fresh
one-row vector selection/proof decision: one concrete seam and its
positive/malformed matrix, with the other two rows excluded. Do not reuse the
rejected scalar-to-vector `LirInsertElementOp` selection or silently select
`LirExtractElementOp`/`LirShuffleVectorOp`.

## Watchouts

Steps 1--8 remain accepted. Do not repeat 811/814 prerequisite work, recover
facts from display text, widen into generic provenance/layout or CFG/PHI, or
claim that the 3038/3038 prerequisite baseline proves a 754 row.

## Proof

The next selection is audit-only until a row-local contract exists. Preserve
the returned 814 proof: fresh build; matching `^backend_` guard 6/6 with no
new failures; representative `scal-to-vec1.c` LLVM emission; and
supervisor-owned full CTest 3038/3038. Do not modify canonical root
`test_before.log` or `test_after.log` for the audit.
