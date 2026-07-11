Status: Active
Source Idea Path: ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory The Adapter Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by inventorying the current `LIR -> BIR` adapter
boundary in `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`.
The note names the public/detail files, implementation split, responsibility
groups, route-local compatibility maps, downstream dependency evidence, and
transient RV64 scan pointers needed for later durable handoff docs.

## Suggested Next

Execute `plan.md` Step 2 by classifying the inventoried responsibilities by
first owning layer and comparing the split against `docs/bir_core_cleanup/`,
`docs/bir_prealloc_fusion/`, and RV64 post-contract evidence.

## Watchouts

- This active idea is umbrella triage; do not edit implementation files or
  treat direct backend case repair as progress.
- Treat ignored `build/` scan artifacts as evidence inputs only, not canonical
  lifecycle state.
- `lowering.hpp` exposes many route-local raw-spelling compatibility maps;
  Step 2 should keep those import-local unless a narrower contract is proven.
- Prepared/prealloc evidence is downstream consumer evidence, not adapter
  ownership evidence.

## Proof

Passed: `git diff --check -- todo.md docs/lir_bir_adapter_boundary`.
No `test_after.log` was created because this docs-only packet delegated a
direct diff-check proof and restricted root-level proof logs.
