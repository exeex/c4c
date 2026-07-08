Status: Active
Source Idea Path: ideas/open/599_pointer_base_plus_offset_selected_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Pointer-Base-Plus-Offset Consumers

# Current Packet

## Just Finished

Activated `ideas/open/599_pointer_base_plus_offset_selected_authority.md` into
`plan.md`.

## Suggested Next

Execute Step 1 from `plan.md`: audit pointer-base-plus-offset producers,
support facts, verifier/decoded-home surfaces, prepared lookup/publication
surfaces, and narrow target consumers enough to choose one representative
shared prepared/prealloc route or record why implementation should stop at
contract state.

## Watchouts

- Keep pointer-value indirect memory-use freshness in idea 600.
- Do not treat home shape, byte delta, range/layout facts, stack/register
  placement, target offset encodability, target operand shape, diagnostics, or
  dumps as selected pointer-arithmetic authority.
- Target-wide RV64/AArch64/x86 migration, semantic GEP target consumption, and
  relocation/materialization semantics are out of scope for this runbook.

## Proof

Lifecycle activation only. Run `git diff --check` before handoff.
