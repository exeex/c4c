Status: Active
Source Idea Path: ideas/open/659_rv64_byval_prepared_call_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Focused Byval Evidence

# Current Packet

## Just Finished

Activated `ideas/open/659_rv64_byval_prepared_call_boundary.md` into
`plan.md` and aligned this execution scratchpad to Step 1.

## Suggested Next

Executor should run Step 1: refresh focused evidence for the nine
byval/prepared call-boundary rows and record the first proven owner or smaller
split before making implementation changes.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, runtime policy,
  timeout settings, or baseline acceptance files.
- Do not merge pointer-local, stack fan-in, AArch64, CLI, static object-data,
  callee-saved GPR, packed-member, or LLVM torture work into this route.
- Treat all nine target rows as probe-first until focused evidence proves the
  selected owner.
- Reject named-case or final-assembly-shape fixes.

## Proof

Lifecycle activation only. No build or tests run.
