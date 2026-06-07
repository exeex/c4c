Status: Active
Source Idea Path: ideas/open/117_aarch64_comparison_fused_compare_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Characterize Comparison Publication Contract Residue

# Current Packet

## Just Finished

Activation complete. `plan.md` now transcribes idea 117 into an executable
runbook, and this `todo.md` points at Step 1 for the first supervisor-delegated
executor packet.

## Suggested Next

Delegate Step 1 - Characterize Comparison Publication Contract Residue.

The executor should inspect `comparison.cpp` and the relevant shared
prepared/BIR prealloc query owners, then update this file with a
helper-to-contract map, candidate shared facts, target-local keep-local notes,
and proof recommendations. No implementation edits are required for Step 1
unless the supervisor explicitly includes them in the packet.

## Watchouts

- Do not edit implementation files during Step 1 unless the delegated packet
  explicitly authorizes it.
- Do not reopen idea 53 repairs without current-code evidence of a new
  uncovered gap.
- Reject same-block producer rescans, raw terminator recovery, BIR-name
  matching, and named fused-compare shortcuts as permanent authority.
- Keep AArch64 condition-code spelling, compare opcode selection, branch
  emission, operand emission, register printing, and scratch register policy
  local.
- Any progress claim before closure must cover fused-compare and
  materialized/current-block publication neighbors.

## Proof

Not run. Activation is lifecycle-only.
