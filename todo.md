Status: Active
Source Idea Path: ideas/open/646_rv64_branch_same_block_home_value_identity_reconciliation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Same-Block Identity Evidence

# Current Packet

## Just Finished

Lifecycle reset activated Step 1 of `plan.md`.

## Suggested Next

Refresh the `src/990127-1.c` same-block RHS `%lv.a` diagnostics and capture
the exact prepared home/value identity mismatch shape.

## Watchouts

- Do not infer value identity from stack offsets, final assembly, source
  syntax, local names, or diagnostics.
- Do not reopen prepared branch stack-source freshness publication from idea
  636.
- Do not absorb RV64 terminator-fragment work from the now-closed idea 645.

## Proof

Lifecycle-only transition. Close gate for idea 645 used the existing focused
8-test before/after CTest scope with `--allow-non-decreasing-passed`:
`passed=8 failed=0 total=8` before and after.
