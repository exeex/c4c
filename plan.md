# LIR-to-BIR Native Label-Address Constant Contract Runbook

Status: Active
Source Idea: ideas/open/770_lir_to_bir_native_label_address_constant_contract.md
Activated from: 768 Step 5, after the synthetic label-address bridge was
rejected by `a89f5f4c6`.

## Purpose

Provide the smallest native representation/lowering route needed for a direct
label-address pointer constant to retain `LirValueId` authority through
indirect-jump consumption.

## Goal

Establish a native direct constant contract that
LLVM can emit as `blockaddress(...)` in a legal direct context while the
compiler retains a real identity-bearing pointer value for consumption.

## Core Rule

`blockaddress` is a pointer constant, not an LLVM instruction. Do not create
an SSA identity with `select`, `gep`, `bitcast`, text recovery, or a dummy
instruction; preserve the native constant through the owned lowering boundary.

## Selected Contract (Step 1 complete)

- The function owns a non-instruction `LirDirectLabelAddressConstant` carrying
  its enclosing `LinkNameId`, `LirBlockId` target, pointer `LirTypeRef`, and
  produced `LirValueId`.
- `LirOperandKind::DirectConstant` consumes that value identity. The printer
  resolves the structured record only in legal direct-operand contexts.
- The selected Raw-BIR passage is `ConstantPayload { BlockId target }`, with a
  builder definition method and foundation verification. The importer
  pre-registers the source value and defines its mapped target before
  terminator lowering; existing `IndirectJumpTerm ValueId` then consumes it.
- Reject invalid, missing, or duplicate produced values; non-pointer types;
  invalid or foreign owners/targets; and a direct-use identity mismatch or
  non-pointer address.

## Read First

- `ideas/open/770_lir_to_bir_native_label_address_constant_contract.md`
- `ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
- `src/backend/bir/lir_to_bir/scalar.cpp` pointer operand lowering
- `src/backend/bir/lir_to_bir/lir_to_bir.cpp` indirect-jump lowering
- `src/backend/bir/core/ir.hpp` `IndirectJumpTerm`

## Non-Goals

- no 768 producer recovery, 764 carrier publication, external integration, or
  automatic-table decay
- no 767/769 changes, Raw-BIR/importer/734 expansion beyond the selected
  native-constant passage, or broad backend work
- no synthetic instruction bridge or rendered-text authority

## Execution Rules

1. Keep the representation boundary explicit: direct constant identity is not
   an instruction result.
2. Change only surfaces required to preserve and consume the selected native
   pointer authority; create a new blocker if an unowned boundary is required.
3. Use direct focused positive and malformed proof; do not select external
   computed-goto cases as the primary capability proof.
4. Build freshly before the focused proof and record the exact return to 768.

## Ordered Steps

### Step 1 - Specify the native direct-constant authority boundary

Goal: select one representation that preserves enclosing-function, target
label, pointer type, and produced `LirValueId` without treating blockaddress as
an instruction.

Actions:

- inspect current LIR operand/value alternatives and scalar lowering dispatch
- identify the smallest representation and validation owners needed for a
  direct identity-bearing label-address constant
- state the exact producer-to-indirect-jump consumption invariants and reject
  unowned Raw-BIR/importer/backend expansion

Completion check:

- one bounded representation/lowering contract is named, including malformed
  authority and pointer-use rejection points, with no fabricated SSA route.

Completed: selected the contract in **Selected Contract (Step 1 complete)**.

### Step 2 - Implement native representation and lowering consumption

Goal: preserve the direct label-address constant as a typed identity-bearing
pointer value through the selected LIR-to-BIR and indirect-jump boundary.

Actions:

- add function-owned non-instruction `LirDirectLabelAddressConstant` and the
  `LirOperandKind::DirectConstant` value-use path, with exact owner/target/
  pointer/value validation
- make the printer emit legal direct `blockaddress(...)` only from that
  structured record in a legal direct operand context
- add only the selected Raw-BIR `ConstantPayload { BlockId target }`, builder
  definition method, foundation verifier checks, and importer pre-registration
  plus mapped-target definition before terminator lowering
- keep existing `IndirectJumpTerm ValueId` consumption; stop for a separate
  blocker before changing `scalar.cpp`, legacy BIR, preparation, MIR, codegen,
  carrier publication, or any broader backend route

Completion check:

- no synthetic bridge remains and the chosen direct constant reaches the
  owned consumption boundary with its required identity intact; malformed
  owner/target/value/type/direct-use boundaries fail closed.

### Step 3 - Prove the owned contract and return to 768

Goal: lock the representation/lowering contract with direct positive and
malformed evidence, then restore 768 at its interrupted producer step.

Actions:

- add direct focused positive proof for the native constant and consumption
  route
- add nearby malformed proof for the selected invalid/missing/foreign
  function, target, produced identity, or pointer-use forms
- run a fresh build and selected focused proof
- record the accepted contract and return 768 to Step 5 only; do not run or
  claim external integration or carrier publication

Completion check:

- focused proof passes, malformed cases reject at the selected boundary, and
  the lifecycle handoff gives 768 the exact native contract needed to repair
  its direct `LabelAddrExpr` producer.
