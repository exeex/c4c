# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.7
Current Step Title: Migrate global and publication consumers

## Just Finished

- Plan Step 2.7 removed the remaining target-local address-materialization
  index fallback from the AArch64 global/publication family. Global address,
  value-home publication, and edge-publication consumers now borrow only
  traversal-attached common lookups with owner/pointer identity validation and
  fail closed when that authority is absent.

## Suggested Next

- Execute Plan Step 3 to run the scoped retirement search, classify any
  surviving route vocabulary, and perform the supervisor-selected broader
  AArch64 validation and review checkpoint.

## Watchouts

- Step 2.7 required no test changes or common contract changes. The singular
  relocation materializer still consumes prepared addressing records directly;
  the removed behavior was the block-index fallback that rescanned those
  records when the common indexed relation was missing.

## Proof

- Passed the exact supervisor-selected proof: `cmake --build --preset default`
  followed by `ctest --test-dir build -j --output-on-failure -R
  '^(backend_codegen_route_aarch64_(got_load_global_prepared_memory|store_global_stack_publication|sret_global_scalar_source_publication|large_stack_global_byte_publication))$'`
  (4/4). Canonical combined proof output is in `test_after.log`; the subset was
  sufficient for this bounded Step 2.7 global/publication-consumer migration.
