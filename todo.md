Status: Active
Source Idea Path: ideas/open/262_phase_f3_x86_compare_join_loadlocal_selected_arm_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Contract Inventory

# Current Packet

## Just Finished

Lifecycle switched from idea 261 to idea 262. Idea 261 is parked because its
supported publication route is blocked by missing production compare-join
selected-`LoadLocal` support, not by fixture construction.

## Suggested Next

Execute Step 1. Inspect the prepared materialized compare-join classification
and render-contract helpers plus the x86 render site in
`src/backend/mir/x86/module/module.cpp`; record the minimal selected
`LoadLocal` extension target in this file.

## Watchouts

- Keep this bounded to same-predecessor-block `LoadLocal` selected arms with
  prepared local-slot addressing.
- Do not revive synthetic `join_transfers` or hand-built publication rows.
- Do not broaden into generic compare-join selected-expression lowering.
- Preserve existing immediate, param, global, pointer-backed global, and
  trailing-immediate compare-join shapes.

## Proof

Lifecycle-only switch. No implementation validation was required.
