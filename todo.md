Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Backend Layout Authority

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Start Step 1 by inventorying backend-owned aggregate layout consumers that
still parse `module.type_decls`, then choose the first concrete conversion
packet.

## Watchouts

Do not migrate `src/backend/mir/`; if MIR `.cpp` files are the only compile
blocker, treat them as compile-target exclusion candidates.

Do not remove `module.type_decls`; structured layout must keep legacy fallback
and parity observation.

## Proof

Not run; lifecycle activation only.
