Status: Active
Source Idea Path: ideas/open/89_aarch64_memory_store_retargeting_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the Existing Retargeting Boundary

# Current Packet

## Just Finished

Activation created the active runbook for Step 1; no implementation packet has
run yet.

## Suggested Next

Delegate Step 1 to an executor. The packet should inspect the target helper
cluster in `memory.cpp`/`memory.hpp`, choose the minimal owner file/header
shape, identify call sites, and write the narrow proof command into this file.

## Watchouts

- Keep the route memory-local; do not publish generic storage, register,
  diagnostic, dispatch, or BIR authority.
- Do not fold store publication, edge-copy, or prepared-wrapper work into this
  idea.
- Do not claim progress through expectation downgrades or helper renames alone.

## Proof

Not run; activation is lifecycle-only.
