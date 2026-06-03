Status: Active
Source Idea Path: ideas/open/108_prepared_select_chain_dump_contract_coverage.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Add Focused Prepared-Printer Tests

# Current Packet

## Just Finished

Step 6 added focused prepared-printer tests for the select-chain/direct-global
dump contract.

Focused proof added in `tests/backend/bir/backend_prepared_printer_test.cpp`:
- New `select_chain_direct_global_dump_contract` fixture builds a
  `load_global -> select -> call argument` path.
- Asserts `prepared-call-plans` argument output carries
  `direct_global_select_chain=yes`, `direct_global_source=selected.arg`,
  `direct_global_root_is_select=yes`, and `direct_global_root_inst=1`.
- Asserts the `--- prepared-select-chain-materializations ---` section is
  present.
- Asserts the selected scalar row contains root fields, nested direct-global
  fields, and source-producer labels:
  `function`, `block`, `value`, `root_is_select`, `root_inst`,
  `direct_global_root_is_select`, `direct_global_root_inst`,
  `source_producer`, `source_producer_block`, and `source_producer_inst`.
- Asserts the direct global load row also carries root/direct-global and
  `source_producer=load_global` labels.

No implementation changes were needed in this packet.

## Suggested Next

Execute Step 7 final validation and close-readiness notes for the prepared
select-chain dump contract.

## Watchouts

- Keep the scalar dump helper-backed; do not reintroduce printer-local lookup
  traversal.
- Store-source visibility remains deferred unless a bounded carried fact
  directly supports it.

## Proof

Passed. Ran:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_printer$'; } > test_after.log 2>&1`

Proof log: `test_after.log`.
