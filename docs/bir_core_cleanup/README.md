# BIR Core Cleanup Handoff

Source idea: `ideas/open/518_bir_core_model_cleanup_umbrella.md`
Plan step: Step 6 - Finalize Analysis And Lifecycle Handoff

This directory is the completed analysis artifact for idea 518. It is
analysis-only: it records ownership, dependency, destination, and follow-up
planning for BIR core cleanup without moving declarations or definitions.

## Artifact Index

| Artifact | Purpose |
| --- | --- |
| `structure_snapshot.md` | Current size, AST query log, symbol counts, selected dependency probes, and raw-text fallback notes. |
| `declaration_inventory.md` | Declaration-family inventory for `bir.hpp`, including likely owner, movement risk, and do-not-move-yet notes. |
| `implementation_inventory.md` | Implementation-family map for `bir.cpp`, printer, validator, and `lir_to_bir/` files, including movement risk. |
| `destination_map.md` | Existing-file redistribution candidates, possible new files, retained central surfaces, include/API risks, and late/no-move regions. |
| `follow_up_ideas.md` | Staged behavior-preserving cleanup ideas with owned files, non-goals, validation expectations, and reviewer reject signals. |

## Acceptance Checklist

| Source idea requirement | Satisfied by |
| --- | --- |
| Explicit, reviewable BIR cleanup plan | `destination_map.md` and `follow_up_ideas.md` |
| Concrete follow-up ideas and owned files | `follow_up_ideas.md` |
| Existing-file redistribution distinguished from new-file splits | `destination_map.md` |
| Records what should not move yet | `declaration_inventory.md`, `destination_map.md`, and `follow_up_ideas.md` |
| Current line-count and dependency snapshot | `structure_snapshot.md` |
| `c4c-clang-tools` query notes and commands | `structure_snapshot.md`, `declaration_inventory.md`, and `implementation_inventory.md` |
| Declaration-family inventory for `bir.hpp` | `declaration_inventory.md` |
| Implementation-family inventory for `bir.cpp` and adjacent owners | `implementation_inventory.md` |
| Risk notes for public API, include churn, and validation scope | `destination_map.md` and `follow_up_ideas.md` |
| No implementation or test-file movement in this umbrella | Confirmed by Step 6 proof command recorded in `todo.md` |

## Lifecycle Handoff

The supervisor can send idea 518 to plan-owner close review. The durable
analysis artifacts are complete enough for a lifecycle decision, and the
follow-up implementation work should be represented as separate staged ideas
instead of expanding this umbrella.
