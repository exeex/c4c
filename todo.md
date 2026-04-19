# Execution State

Status: Active
Source Idea Path: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory The Current Phi Legalization Route
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Activated idea 63 into `plan.md` after closing idea 62. No executor packet has
run on this route yet.

## Suggested Next

Start Step 1 by auditing `src/backend/prealloc/legalize.cpp`,
`src/backend/prealloc/prealloc.hpp`, and `src/backend/prealloc/regalloc.cpp`
for select-shaped phi recovery, slot-backed fallback, and any existing
authoritative phi-transfer handoff surfaces.

## Watchouts

- Do not treat one additional phi testcase shape as success.
- Keep typed semantic ids as the public identity boundary.
- Do not silently fold branch/join ownership work from idea 62 back into this
  route.
- Avoid slot-only fallback growth unless it is a temporary legality aid inside
  a more general transfer model.

## Proof

Lifecycle activation only. No build or test command has run for idea 63 yet.
