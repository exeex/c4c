# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Add and index the complete D2 subordinate contract

## Just Finished

- Plan Step 8 added `passes/call_lowering/README.md` as the complete local D2
  owner and indexed it exactly once from the root stage order and from the
  local target-aware pass index.
- Closed D2's admitted call-transport schema, exact D1/C3/C4/product keys,
  shared target-data-driven ABI-rule selection, new revision and stable-ID
  rules, invalidation boundary, failure atomicity, D3/D4 adjacency, and legacy
  disposition.
- Reduced the D1 text to a handoff and aligned the C4 planner, pseudo schema,
  verifier profile, and D4 boundary with the dedicated D2 owner.

## Suggested Next

- Execute Plan Step 9.1, "Choose the BIR-owned D5 parallel-copy realization
  route."

## Watchouts

- D2 invalidates any revision-bound projected constraint record but deliberately
  does not choose the single projection mechanism; that ownership remains Plan
  Step 10.
- Step 9.1 must choose a BIR-owned realization route for D5 `ParallelCopy`
  cycles before E4 without moving repair or temporary creation into MIR.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && test -f src/backend/bir/passes/call_lowering/README.md && rg -n 'D2|CallPlan|AbiPlan|GenericCall|AbiArgMove|AbiCall|revision|fingerprint|invalidat|transaction|rollback|failure|D1|D3|D4' src/backend/bir/passes/call_lowering/README.md && test "$(rg -o 'passes/call_lowering/README\.md' src/backend/bir/README.md | wc -l)" -eq 1 && test "$(rg --files src/backend/bir -g '*.md' | wc -l)" -eq 44`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
