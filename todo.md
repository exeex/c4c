Status: Active
Source Idea Path: ideas/open/111_store_source_publication_dump_visibility.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Prepared-Printer Contracts

# Current Packet

## Just Finished

Step 3 added focused prepared-printer contracts for the retained
store-source publication dump output.

Coverage details:
`backend_prepared_printer_test.cpp` now asserts the
`--- prepared-store-source-publications ---` section and a concrete
source-producer row from `call_argument_source_shape_dump_contract`:
`inst=4 source=derived.seed status=available intent=store_local_publication`
with `source_producer=binary`, `source_producer_block=entry`,
`source_producer_inst=3`, `source_binary=yes`, all other source-shape booleans
negative, and no direct-global dependency.

The direct-global select-chain fixture now stores `selected.arg` into
`lv.selected`; the prepared-printer contract asserts the resulting
store-source row from `select_chain_direct_global_dump_contract`:
`inst=2 source=selected.arg status=available intent=store_local_publication`
with `source_producer=select_materialization`,
`source_producer_block=entry`, `source_producer_inst=1`,
`source_select=yes`, `direct_global_select_chain=yes`,
`direct_global_root_is_select=yes`, and `direct_global_root_inst=1`.

## Suggested Next

Run Step 4 acceptance tightening: inspect the diff for expectation weakening,
header-only coverage, printer-side semantic re-derivation, or scope expansion,
then run any supervisor-selected broader validation needed beyond the focused
prepared-printer/store-source subset.

## Watchouts

- The new direct-global prepared-printer fixture keeps the existing call
  argument and select-chain materialization checks while adding one local store;
  row indices expected by the existing select-chain checks remain unchanged.
- Store-source planning fail-closed behavior remains covered by
  `backend_store_source_publication_plan`; this packet did not weaken those
  expectations.
- No implementation files were touched in this Step 3 packet.

## Proof

Ran delegated Step 3 proof:
`bash -lc 'git diff --check && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_printer|backend_store_source_publication_plan)$"' > test_after.log 2>&1`;
passed 2 of 2 tests. Proof log: `test_after.log`.
