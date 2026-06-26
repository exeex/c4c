Status: Active
Source Idea Path: ideas/open/375_rv64_object_route_scalar_compare_trunc_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Scalar Compare Trunc Facts

# Current Packet

## Just Finished

Activated `ideas/open/375_rv64_object_route_scalar_compare_trunc_lowering.md`
as the active runbook.

## Suggested Next

Execute Plan Step 1 by auditing semantic and prepared compare/trunc facts for
`src/20000217-1.c`, especially `showbug`.

Record the concrete predicate, operand homes, trunc source and destination
widths, publication destination, existing `eq`/`ne` coverage boundary, and the
first supportable implementation shape.

## Watchouts

- Do not reopen frame-slot address call-argument materialization, pointer-value
  local memory, or frame-slot payload-value call arguments; ideas 372, 368, and
  373 already own those routes.
- Do not absorb non-register parameter homes, aggregate `va_arg`, byval homes,
  terminators, CFG reconstruction, or source-shape shortcuts.
- Progress must come from prepared compare/trunc metadata, not testcase names,
  hard-coded constants, value ids, frame slots, or registers.
- Put audit logs under `build/agent_state/`; root `test_after.log` is reserved
  for delegated proof.

## Proof

Lifecycle-only activation. No build proof required.
