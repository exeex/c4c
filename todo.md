Status: Active
Source Idea Path: ideas/open/108_prepared_select_chain_dump_contract_coverage.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement Remaining Scalar And Source-Producer Dump Visibility

# Current Packet

## Just Finished

Step 5 confirmed the Step 4 route already implements the remaining scalar and
source-producer dump visibility; no additional code changes were needed in
this packet.

Confirmed dump surfaces:
- `--- prepared-select-chain-materializations ---` is wired after
  `prepared-call-plans`.
- Scalar rows include root fields:
  `function=<function>`, `block=<block>`, `value=<root-value>`,
  `root_is_select=yes|no`, and `root_inst=<index>`.
- Scalar rows append nested direct-global fields when the helper-reported
  materialization contains a direct-global dependency:
  `direct_global_select_chain=yes`,
  `direct_global_root_is_select=yes|no`, and
  `direct_global_root_inst=<index>`.
- Scalar rows append bounded source-producer provenance for the same selected
  value root: `source_producer=<kind>`,
  `source_producer_block=<block>`, and `source_producer_inst=<index>`.
- `prepared-call-plans` argument lines still print carried call-argument
  direct-global dependency labels:
  `direct_global_select_chain=yes`, `direct_global_source=<value>`,
  `direct_global_root_is_select=yes|no`, and `direct_global_root_inst=<index>`.

Implementation boundary:
- The scalar section formats facts returned by
  `make_prepared_edge_publication_source_producer_lookups`,
  `find_indexed_prepared_edge_publication_source_producer`, and
  `find_prepared_scalar_select_chain_materialization`.
- `select_chain_lookups.cpp` remains the single helper implementation point;
  no printer-local lookup traversal was added.
- Store-source visibility remains deferred.

## Suggested Next

Execute Step 6 focused prepared-printer tests for call-argument direct-global
labels and scalar select-chain rows, including root fields, nested
direct-global fields, and source-producer labels.

## Watchouts

- Do not weaken expectations or replace the helper-backed scalar section with
  printer-local traversal.
- The scalar section prints positive materialization rows only.
- Store-source visibility remains out of scope unless a bounded carried fact
  directly supports it.

## Proof

Passed. Ran:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_printer$'; } > test_after.log 2>&1`

Proof log: `test_after.log`.
