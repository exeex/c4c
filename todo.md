Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebaseline Remaining Inline-Asm Gap Matrix

# Current Packet

## Just Finished

Lifecycle reset: replaced the exhausted Step 1-6 runbook with a focused
remaining-scope runbook for the same open source idea.

## Suggested Next

Execute Plan Step 1 by recording the remaining inline-asm gap matrix and the
first narrow executor packet for named operands, clobbers, memory/address
constraints, or tied-output coallocation.

## Watchouts

- Do not close `ideas/open/240_aarch64_inline_asm_machine_nodes.md` yet.
- Preserve the completed positional register, single output, numeric tie,
  integer immediate, `%wN`/`%xN`, and side-effect support.
- Split a new open idea only if clobber ingress, memory/address constraints, or
  tied-output coallocation proves to require distinct source/LIR or allocator
  ownership.

## Proof

Lifecycle-only update. No implementation validation was run by the plan owner.
