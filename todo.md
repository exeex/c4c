# Current Packet

Status: Active
Source Idea Path: ideas/open/770_lir_to_bir_native_label_address_constant_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement native representation and lowering consumption

## Just Finished

- Step 1 selected the function-owned non-instruction
  `LirDirectLabelAddressConstant`: enclosing `LinkNameId`, `LirBlockId` target,
  pointer `LirTypeRef`, and produced `LirValueId`. Its only direct use is
  `LirOperandKind::DirectConstant`; the printer resolves it only in legal
  direct operand contexts. The bounded passage is Raw-BIR
  `ConstantPayload { BlockId target }`, builder definition, foundation
  verification, importer pre-registration/mapped-target definition before
  terminator lowering, then existing `IndirectJumpTerm ValueId` consumption.

## Suggested Next

- Implement only the selected LIR direct-constant record/value use, legal
  direct printer emission, and the minimal Raw-BIR/builder/verifier/importer
  passage. Reject invalid/missing/duplicate values, non-pointer types,
  invalid/foreign owner or target, and mismatched direct-use identity or
  non-pointer address. Preserve `IndirectJumpTerm ValueId` consumption.

## Watchouts

- LLVM 19 does not accept `%x = blockaddress(...)` as an instruction. Do not
  use `select`, `gep`, `bitcast`, text recovery, or dummy results. Do not touch
  `scalar.cpp`, legacy BIR, preparation, MIR, codegen, carrier publication,
  768 producer recovery, 764, external cases, table decay, or 767/769. The
  selected Raw-BIR/importer passage is the sole exception; stop for a separate
  blocker before any broader backend work.

## Proof

- Lifecycle evidence: `a89f5f4c6` rejects the synthetic bridge; prior 768
  progress and accepted prerequisite references are preserved in its resumption
  record. The focused proof is the `backend_lir_to_bir_interface` fixture with
  direct positive and malformed coverage. Exact narrow command selection awaits
  the supervisor; do not assign it to the executor. No regression logs change
  for this packet.
