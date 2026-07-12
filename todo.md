Status: Active
Source Idea Path: ideas/open/715_pass_ready_bir_schema_and_legacy_quarantine_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current LIR-to-BIR Schema

# Current Packet

## Just Finished

- Completed plan.md Step 1: created the evidence-backed current LIR-to-BIR
  schema, identity, authority, consumer, mutation-assumption, and hazard
  inventory in `docs/backend/pass_ready_bir/01_current_lir_to_bir_schema.md`.

## Suggested Next

- Execute plan.md Step 2 and classify every Step 1 field family while defining
  the legacy compatibility quarantine.

## Watchouts

- Preserve the distinction between module-owned emitted side tables and
  consumer-built route 1--8 pointer/index views.
- The hybrid ID/name/pointer/index model makes every post-publication structural
  mutation unsafe without rebuilding dependent views and prepared plans.

## Proof

- Supervisor-selected documentation proof passed:
  `git diff --check && test -f docs/backend/pass_ready_bir/01_current_lir_to_bir_schema.md && rg -n "Module|Function|Block|Inst|Terminator|RAUW|split|redirect|authority|consumer" docs/backend/pass_ready_bir/01_current_lir_to_bir_schema.md`.
- This documentation-only packet does not produce `test_after.log`; no build or
  test subset was delegated.
