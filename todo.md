# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Close and inventory the store-source boundary

## Just Finished

- Step 2.1 — derived cast/select named-producer applicability from producer
  family rather than the presence of a producer block-label string. Both
  families now fail closed when that identity is absent, with focused negative
  coverage added for each family.
- Residual family inventory: load-local, cast, binary, and select
  materialization require available, kind/index/block/value-matching named BIR
  producer evidence; load-global remains payload/home/access/order checked but
  is not currently evidence-applicable; immediate is authorized only by the
  separate global-immediate path; unknown always fails closed.
- Caller inventory: normal population and pending-global planning populate
  named evidence and the BIR block label; direct-global planning initially has
  no producer and therefore shares the fail-closed metadata boundary (with
  immediate handled separately); fixed-formal planning delegates unchanged to
  the same planner and inherits the boundary from its supplied store inputs.

## Suggested Next

- Execute the bounded Step 2.1 AArch64 compatibility-adapter packet: prefer
  consuming the unique precomputed prealloc store-source publication record;
  only if that cannot be reached without broader target migration, transport
  the named BIR producer result and block-label identity already available in
  `BlockLoweringContext` into the common prealloc planner. Then rerun the exact
  delegated backend proof.

## Watchouts

- The delegated proof remains incomplete because the AArch64
  `plan_store_local_source_publication` compatibility caller supplies a
  cast producer but neither `source_producer_evidence` nor
  `source_producer_block_label`. The supported prepared-memory and instruction
  dispatch cast-publication positives consequently fail closed as required by
  the new boundary.
- Do not weaken the family-derived requirement or downgrade those supported
  positives. The authorized exception is adapter-only: consume or transport a
  prealloc-owned fact/input, with no target-side selection authority,
  synthesized evidence, testcase/producer-shape exception, or broader target
  materializer migration.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: build passed; 307/309 backend tests passed. Failures:
  `backend_aarch64_prepared_memory_operand_records` and
  `backend_aarch64_instruction_dispatch`, both supported cast-publication paths
  through the unowned target-side caller described above. Canonical proof log:
  `test_after.log`.
