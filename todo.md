# Current Packet

Status: Active
Source Idea Path: ideas/open/757_lir_computed_goto_address_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish computed-goto address authority

## Just Finished

- Step 1 complete: `LirIndirectBrOp.addr_value` now carries the preserved
  current-function pointer `LirValueId`; `addr` is verified only as its display
  mirror, and missing, invalid, foreign, non-pointer, and misleading-display
  authority rejects before printing/downstream use.

## Suggested Next

- Supervisor: accept this Step 1 slice and route the typed `addr_value` plus
  ordered `successors` handoff to the bounded idea 734 receiver packet.

## Watchouts

- Idea 734 may consume `LirIndirectBrOp.addr_value` only after LIR verification;
  `addr`, labels, and printer output remain non-authoritative display mirrors.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1; detailed focused output: `test_after.log`.
- Supervisor: select and record broader/full acceptance separately.
