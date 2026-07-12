# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and migrate AArch64 dispatch authority

## Just Finished

- None; idea 709 has just been activated.

## Suggested Next

- Execute Plan Step 1 at `src/backend/mir/aarch64/codegen/dispatch.cpp`, inventorying executable route authority and migrating one coherent dispatch family to existing common named/prepared queries.

## Watchouts

- Do not recreate route indexes or producer reasoning in AArch64 helpers. Stop for lifecycle review if the common query contract cannot express required authority.

## Proof

- Pending executor build and supervisor-selected focused AArch64 proof.
