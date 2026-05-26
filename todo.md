Status: Active
Source Idea Path: ideas/open/39a_aarch64_store_source_semantic_residue_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Semantic Residue And Prepared Fact Gaps

# Current Packet

## Just Finished

Lifecycle repaired after idea 39 Step 1 found semantic blockers to mechanical
fold-back. Created and activated the prerequisite source idea for remaining
AArch64-local store-source semantic residue. Parked idea 39 until this
prerequisite is complete.

## Suggested Next

Step 1 - Inventory Semantic Residue And Prepared Fact Gaps: map each blocked
`memory_store_sources.*` helper to the source fact it currently recovers
locally, identify existing or missing prepared authority, record external
callers and focused tests, and recommend the first semantic implementation
packet.

## Watchouts

Do not fold `memory_store_sources.*` into `memory.cpp` in this plan. Do not
rename local source rediscovery as prepared authority. Preserve diagnostics,
fail-closed behavior, ABI/memory semantics, and existing supported behavior.

## Proof

No build required for lifecycle-only repair.
