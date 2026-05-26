Status: Active
Source Idea Path: ideas/open/19_x86_riscv_prepared_edge_publication_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Shared Prepared Edge Publication

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: audited the shared prepared edge-publication
records/helpers in `prepared_lookups`, `publication_plans`,
`formal_publications`, prepared printer output, and focused lookup/publication
tests. The shared prepared edge-publication contract is target-neutral for
x86/RISC-V consumption: records publish prepared labels, value ids, BIR source
values, destination/source homes, generic storage/home kinds, out-of-SSA
parallel-copy ownership, execution-site/block labels, cycle/temp ordering, and
source-producer BIR instruction facts. No hidden AArch64 register bank, register
number, ABI lane, instruction-selection, or emitter policy assumptions were
found in the shared records/helpers.

## Suggested Next

Proceed to Step 2 by wiring the x86/RISC-V consumer side to read the shared
edge-publication lookup facts instead of creating a duplicate edge-copy
authority.

## Watchouts

- Do not treat AArch64 success as proof that the shared contract is
  target-neutral.
- Do not move target emission details into shared prepare or BIR.
- Do not create a duplicate x86/RISC-V edge-copy authority.
- Keep routine execution findings in this file unless the active runbook or
  source idea truly needs to change.
- Shared prepared facts may include opaque target-produced register spellings
  from value homes/moves; consumers should interpret those through their own
  target register model rather than assuming AArch64 names.

## Proof

No build required per packet because this was an audit-only `todo.md` update.
Read-only audit commands inspected `prepared_lookups`, `publication_plans`,
`formal_publications`, prepared printer files, value-location records, and
focused prepared edge-publication tests; no root-level logs were created.
