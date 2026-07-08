Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Pointer BinaryInst Ownership

# Current Packet

## Just Finished

Plan-owner regenerated `plan.md` after the prior Step 4 classification
exhausted the narrow integer `BinaryInst` route. Idea 612 remains open and
active, but the runbook is now limited to pointer `BinaryInst` /
address-authority instruction-fragment consumers.

## Suggested Next

Executor should run Step 1 from `plan.md`: refresh current pointer
`BinaryInst` / address-authority diagnostics, separate rows by first owner,
and record the selected Step 2 packet with positive rows, negative guard rows,
and the exact backend proof command before code changes.

## Watchouts

- Keep `src/931110-1.c` / scalar `ashr` as a negative guard unless refreshed
  evidence finds broader same-family scalar `ashr` breadth.
- Do not mix pointer `BinaryInst` / address-authority work with idea 614
  pointer local-memory consumption, cast rows, ABI/call rows, select
  publication, branch freshness, move-bundle, global/runtime, inline asm,
  terminator, or policy-owned residuals.
- Do not weaken expectations, unsupported markers, allowlists, timeout
  behavior, runtime handling, or GCC torture classification metadata.

## Proof

Lifecycle-only rewrite; no code validation was required. Latest accepted
backend proof before this lifecycle reset was rolled forward into
`test_before.log` by the supervisor.
