Status: Active
Source Idea Path: ideas/open/34_aarch64_store_source_publication_planning.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish The Selected Store-Source Fact Upstream

# Current Packet

## Just Finished

Step 2 published the selected store-local cast source-producer fact upstream.
`PreparedEdgePublicationSourceProducer` is now a public prepared lookup fact,
`PreparedFunctionLookups` exposes `edge_publication_source_producers`, and
`PreparedStoreSourcePublicationPlan` can carry the producer kind, block,
instruction index, and producer instruction pointer via
`PreparedEdgePublicationSourceProducerKind::Cast` without target codegen
rediscovery.

Focused shared coverage in
`tests/backend/mir/backend_store_source_publication_plan_test.cpp` builds a
minimal prepared function with a same-block cast feeding a store-local source,
queries the prepared source-producer lookup before target emission, and proves
the store-source publication plan carries the cast producer fact.

## Suggested Next

Execute Step 3 for the selected cast-producer target: convert the AArch64
store-local cast consumer to use the prepared store-source producer fact and
remove the corresponding same-block cast rediscovery from the AArch64 path.

## Watchouts

- Do not merge `memory_store_sources.*` into `memory.cpp` during this semantic
  migration.
- Keep the next packet on the existing `PreparedEdgePublicationSourceProducer`
  fact; do not add AArch64-only rediscovery under a new helper name.
- Do not claim progress through expectation rewrites, unsupported downgrades,
  or fixture-specific matching.
- This packet intentionally did not convert AArch64 emission or touch
  `src/backend/mir/aarch64/codegen/memory_store_sources.*`.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_x86_store_source_publication_plan_reuse|backend_aarch64_prepared_memory_operand_records)$' | tee test_after.log`

Result: passed. Build completed, and all 3 targeted tests passed. Proof log:
`test_after.log`.
