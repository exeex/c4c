# Current Packet

Status: Active
Source Idea Path: ideas/open/752_lir_local_object_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the selected local-object authority contract
你該做code review了

## Just Finished

- Step 1 VLA repair: the pointer-slot `LirStoreOp` now carries the hoisted slot
  authority that matches its pointer operand, while the dynamic alloca retains
  its own result authority. Focused coverage identifies that store by the
  dynamic alloca value ID and checks the pointer-authority ID equality.

## Suggested Next

- Supervisor: review this bounded Step 1 repair and decide the next packet.
  Do not start Step 2 malformed-authority checks or Step 3 handoff work until
  this repair is accepted.

## Watchouts

- Stack restore is emitted only on a backward goto with a VLA lifetime route;
  ordinary VLA fixtures do not cover it. The VLA pointer-slot store is selected
  through its dynamic-allocation value ID, never its rendered spelling. Keep
  Raw-BIR/importer, memory/va, aggregate/vector, PHI/CFG, and target-lowering
  outside this packet.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log` passed (5/5). `test_after.log` is the required proof log.
