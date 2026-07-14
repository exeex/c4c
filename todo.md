# Current Packet

Status: Active
Source Idea Path: ideas/open/770_lir_to_bir_native_label_address_constant_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement native representation and lowering consumption

## Just Finished

- Step 2 implemented the function-owned non-instruction
  `LirDirectLabelAddressConstant`, its identity-only direct operand and legal
  indirect-branch printer resolution, plus Raw-BIR label-address payload,
  builder definition, verifier ownership/type checks, and importer
  pre-registration/definition before terminator lowering. Existing
  `IndirectJumpTerm` consumption remains through the source-value map. Focused
  coverage admits the direct route and rejects invalid, duplicate, nonpointer,
  invalid-owner, foreign-target, and mismatched direct-use/address cases.

## Suggested Next

- Supervisor to select the next bounded Step 770 packet; this packet does not
  expand into producer lowering, legacy BIR, preparation/MIR/codegen, or other
  label-address families.

## Watchouts

- LLVM 19 does not accept `%x = blockaddress(...)` as an instruction. This
  packet stays a function-owned constant route; do not broaden into synthetic
  bridges, producer lowering, legacy BIR, preparation/MIR/codegen, or carrier
  publication.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_lir_to_bir_interface$' >
  /tmp/native_label_address_step2.log`. The supervisor-selected focused proof
  is sufficient for this bounded packet; log: `/tmp/native_label_address_step2.log`.
