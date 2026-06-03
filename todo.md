Status: Active
Source Idea Path: ideas/open/108_prepared_select_chain_dump_contract_coverage.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Prepared Dump Visibility

# Current Packet

## Just Finished

Step 3 correction completed: removed the printer-local duplicate
source-producer/select-chain traversal and kept only the already-carried
call-argument direct-global dependency dump surface.

Changed route:
- `prepared-call-plans` argument lines now print carried call-argument
  direct-global dependency labels when
  `find_prepared_call_argument_direct_global_select_chain_dependency` returns a
  dependency: `direct_global_select_chain=yes`,
  `direct_global_source=<value>`, `direct_global_root_is_select=yes|no`, and
  `direct_global_root_inst=<index>`.

Correction details:
- The attempted `prepared-select-chain-materializations` section was removed
  because implementing it in `prepared_printer.cpp` without linking
  `prepared_lookups.cpp`/`publication_plans.cpp` required duplicating
  `make_prepared_edge_publication_source_producer_lookups`,
  `find_indexed_prepared_edge_publication_source_producer`, direct-global
  recursion, and scalar materialization logic.
- Calling the existing non-inline helpers directly from the top-level prepared
  printer is not linkable under the current backend test graph: existing
  printer-only fixtures such as `backend_prepare_phi_materialize_test` compile
  and link `prepared_printer.cpp` and selected printer files without the lookup
  implementation objects.
- No prepared lookup APIs, target lowering, MIR, or call-planning policy were
  changed.
- Store-source dump remains deferred.

## Suggested Next

Execute Step 4 focused proof for the carried call-argument direct-global
dependency labels. Treat scalar select-chain materialization section coverage as
blocked until the prepared-printer linkage/model can consume existing lookup
helpers without duplicating lookup authority.

## Watchouts

- Do not restore printer-local select-chain/source-producer traversal.
- A scalar select-chain section needs either a prepared-printer link model that
  includes the existing lookup/publication helper implementation objects or a
  separately carried prepared-module dump surface.
- Do not broaden Step 4 into store-source visibility unless a prepared-module
  carried fact already provides a bounded dump surface.

## Proof

Passed. Ran:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_printer$'; } > test_after.log 2>&1`

Proof log: `test_after.log`.
