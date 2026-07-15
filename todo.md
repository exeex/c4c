# Current Packet

Status: Active
Source Idea Path: ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory carrier surfaces and prove structural-probe feasibility

## Just Finished

- Lifecycle switch from 783 Step 3: its focused baseline and three-seam
  inventory are preserved in the source resumption record. No carrier,
  implementation, or test change has been accepted in this packet.

## Suggested Next

- Plan Step 1: inventory the shared native current-function carrier surfaces
  and determine from source-level evidence whether PHI incoming value transport
  is indispensable to the three focused structural probe boundaries.

## Watchouts

- Keep the decision limited to native operand/result transport. PHI
  predecessor/edge identity and verification remain owned by 751.
- Do not use text recovery or absorb Raw-BIR/importer, backend, target lowering,
  MIR, emission, broad generic-expression redesign, or 782 helper fields.

## Proof

- Preserved accepted guard: `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1 in
  `test_after.log`; `test_before.log` and `test_after.log` match. No code or
  test change is accepted after that proof.
