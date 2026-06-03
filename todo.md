Status: Active
Source Idea Path: ideas/open/108_prepared_select_chain_dump_contract_coverage.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Resolve Scalar Dump Linkage Or Carry-Fact Model

# Current Packet

## Just Finished

Step 4 resolved the scalar select-chain dump route without restoring
printer-local lookup authority.

Selected route:
- Split the existing source-producer/select-chain helper authority into
  `src/backend/prealloc/select_chain_lookups.cpp`.
- Keep existing consumers on exported helper APIs:
  `make_prepared_edge_publication_source_producer_lookups`,
  `find_indexed_prepared_edge_publication_source_producer`,
  `find_prepared_direct_global_select_chain_dependency`, and
  `find_prepared_scalar_select_chain_materialization`.
- Add a narrow exported root helper,
  `find_prepared_select_chain_source_producer`, so
  `publication_plans.cpp` can continue to consume the same authority after the
  split.
- Add `src/backend/prealloc/prepared_printer/select_chains.cpp` as a printer
  formatting section that only iterates candidate BIR instruction results and
  formats facts returned by the existing helpers.
- Wire `--- prepared-select-chain-materializations ---` after
  `prepared-call-plans`.
- Update the one manual printer-only fixture target,
  `backend_prepare_phi_materialize_test`, to link the new narrow helper unit
  and printer section instead of dragging in broad `prepared_lookups.cpp`,
  `publication_plans.cpp`, `call_plans.cpp`, or formal-publication planning.

Retained Step 3 surface:
- `prepared-call-plans` argument lines print carried direct-global dependency
  labels: `direct_global_select_chain=yes`, `direct_global_source=<value>`,
  `direct_global_root_is_select=yes|no`, and `direct_global_root_inst=<index>`.

Rejected routes:
- No printer-local source-producer/direct-global/scalar traversal.
- No broad prepared-printer link against all lookup/publication/call-planning
  objects for the manual BIR fixture.
- No store-source dump expansion in this packet.

## Suggested Next

Execute Step 5: implement or tighten the remaining scalar/source-producer dump
visibility if any label gaps remain, then Step 6 should add focused
prepared-printer assertions for call-argument direct-global labels and scalar
select-chain rows.

## Watchouts

- The scalar section prints only positive materialization rows returned by
  `find_prepared_scalar_select_chain_materialization`; absence still means no
  dumpable prepared fact was available.
- Source-producer labels are limited to the scalar select-chain row.
- Keep `select_chain_lookups.cpp` as the single helper implementation point;
  do not reintroduce duplicate lookup logic into printer files.
- Store-source visibility remains deferred unless a bounded carried fact
  directly supports it.

## Proof

Passed. Ran:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_printer$'; } > test_after.log 2>&1`

Proof log: `test_after.log`.
