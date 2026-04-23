Status: Active
Source Idea Path: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Contract Surface Audit
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

none

## Suggested Next

Step 1: audit the current `out_of_ssa` contract surface in `src/backend/prealloc/prealloc.hpp`,
`src/backend/prealloc/out_of_ssa.cpp`, and `src/backend/prealloc/prepared_printer.cpp`, then capture the first
bounded implementation packet from that audit.

## Watchouts

- Keep phi elimination and parallel-copy authority in `out_of_ssa`; do not push semantics back into `legalize`.
- Treat grouped-register authority and prepared frame/stack/call work as separate ideas, not scope for this runbook.

## Proof

Lifecycle repair only; no build or tests run.
