# Current Packet

Status: Active
Source Idea Path: ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Bind the minimal generic carrier contract

## Just Finished

- Plan Step 2 completed the minimum carrier contract: AArch64 GP/FP helper
  loads, GEPs, and join inputs plus AMD64 register/stack helper results retain
  native `LirOperand` authority. `LirPhiOp` now transports only its value half
  as `LirOperand`; labels remain strings and no PHI verification or edge work
  was added. Focused structural probes cover all three chains.

## Suggested Next

- Plan Step 3: review the accepted three-chain structural evidence, publish the
  783 handoff, and explicitly retain the value-only PHI decision as transport
  only rather than PHI completion.

## Watchouts

- 785 does not authorize HFA ptrmask conversion, unrelated generic call/argument
  changes, PHI verification, predecessor/edge identity, Raw-BIR/importer,
  backend, target lowering, MIR, emission, or broad generic-expression work.
- `LirPhiIncoming::label` remains presentation-only; do not infer predecessor
  or CFG semantics from this carrier. Do not recover identity from text.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1. Full
  output: `test_after.log`; this is the supervisor-selected focused proof.
