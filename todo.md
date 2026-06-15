Status: Active
Source Idea Path: ideas/open/278_phase_f5_memory_accesses_byte_offset_coverage_gate.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove The Selected Byte-Offset Drift Row

# Current Packet

## Just Finished

Completed Step 2 of `plan.md` by adding the selected same-block public
byte-offset drift proof to
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`.

The new row drives the real RISC-V dynamic `LoadLocal` memory-source consumer
through
`consume_edge_publication_move_intent(..., &route5_memory_edge)`. The selected
publication, selected position row, and Route 3 / Route 5 authority remain at
offset `12`, while a second same-block public `PreparedMemoryAccess` for the
same `%src` value is visible at offset `16`. The consumer fails closed as
`UnsupportedSourceHome`, emits no instruction, records no source-memory offset,
and keeps Route 5 / Route 3 agreement true for the selected offset `12` row.

## Suggested Next

Step 3 should prove or record that compatible dynamic stack-source `LoadLocal`
behavior is preserved after the new same-block duplicate row: exact
`lw a1, 12(s2)` remains available for the non-duplicated fixture, with selected
offset `12` source-memory facts and agreeing Route 3 / Route 5 authority.

## Watchouts

Do not claim exhaustive byte-offset coverage from this one row. The proof is
limited to the RISC-V same-consumer public lookup path and keeps fallback,
status/debug, route-debug, helper/oracle, prepared-printer, wrapper,
exact-output, and baseline contracts unweakened.

## Proof

Passed. Proof command run exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'; } > test_after.log 2>&1
```

`test_after.log` is the proof log. Result: targeted
`backend_riscv_prepared_edge_publication` CTest passed 1/1.
