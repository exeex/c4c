Status: Active
Source Idea Path: ideas/open/694_bir_route_index_retirement_umbrella.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Write Ordered Follow-Up Plan

# Current Packet

## Just Finished

Step 3: Write Ordered Follow-Up Plan completed. Added
`docs/bir_route_index_retirement/ordered_followup_plan.md`, ordering follow-up
families by dependency and blast-radius reduction across route facade
contraction and named aliases, consumer migration, publication boundary
cleanup, stack/frame/value-home handoff cleanup, test/dump contract cleanup,
and residual stack authority revisit. The handoff names the first proof surface
and rollback point for each family and records deferred or unassigned work,
including the parked status for ideas 647 and 655.

## Suggested Next

Start Step 4 in `plan.md`: generate focused `ideas/open/` follow-up ideas from
the ordered handoff, keeping each idea to one owning layer, one first consumer
or producer boundary, one proof surface, and explicit reviewer reject signals.

## Watchouts

Keep idea generation out of Step 3 output. Route-numbered APIs remain private
compatibility during migration, and ideas 647 and 655 remain parked unless
positive prepared producer evidence appears above route dumps.

## Proof

Docs-only proof ran:
`test -f docs/bir_route_index_retirement/ordered_followup_plan.md && rg -n "route facade|named aliases|consumer migration|publication boundary|stack/frame/value-home|test/dump|proof surface|rollback|deferred|unassigned|647|655" docs/bir_route_index_retirement/ordered_followup_plan.md`

No build was required for this documentation-only packet. The delegated proof
does not write `test_after.log`; no root-level log file was created.
