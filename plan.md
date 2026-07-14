# LIR-to-BIR Native Label-Address Constant Store-Consumer Repair Runbook

Status: Active
Source Idea: ideas/open/770_lir_to_bir_native_label_address_constant_contract.md
Reopened from: closed 770 after 768 Step 5 found the unproven ordinary
`LirStoreOp` direct-constant consumer boundary.

## Purpose

Repair the bounded native direct label-address constant contract so the direct
pointer value can legally feed its ordinary pointer-store initializer consumer
without becoming an instruction-shaped SSA bridge.

## Goal

Preserve the accepted function-owned `LirDirectLabelAddressConstant` and make
its `LirOperandKind::DirectConstant` use legal, printable, and verifiable as
`LirStoreOp.val` for a pointer store.

## Core Rule

`blockaddress` remains a direct pointer constant, not an instruction. Extend
only the established direct-constant consumer contract; do not manufacture an
SSA identity with `select`, `gep`, `bitcast`, text recovery, or a dummy result.

## Read First

- `ideas/open/770_lir_to_bir_native_label_address_constant_contract.md`
- `ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
- `src/codegen/lir/lir_printer.cpp` `LirStoreOp` rendering
- `src/codegen/lir/verify.cpp` function value-use and store validation
- accepted 770 commits `e8a0f70b4` and `6803f8c25`

## Non-Goals

- no 768 `LabelAddrExpr` producer changes, frontend carrier publication, or
  764/external integration work
- no automatic-table `DeclRef` decay, 767/769 reopening, or changes to their
  accepted contracts
- no scalar/broad backend, Raw-BIR/importer, MIR, codegen, or 734 expansion
  beyond the already accepted direct-constant passage
- no synthetic bridge, rendered-text authority, testcase branching, verifier
  relaxation, or expectation downgrade

## Execution Rules

1. Preserve the accepted direct constant -> Raw-BIR -> `IndirectJumpTerm`
   route and proof; the repair adds only the legal ordinary store consumer.
2. The store printer must resolve the same structured function-owned constant,
   not recover or parse printed text.
3. Verification must accept only the matching valid direct constant in the
   enclosing function and reject invalid, foreign, missing, duplicate, wrong
   type, or mismatched value authority/use.
4. Use focused positive and nearby malformed coverage; build freshly before
   the selected proof. Stop for a separate blocker before any unowned route.

## Ordered Steps

### Step 1 - Preserve the accepted first contract and isolate the store gap

Goal: confirm that the existing direct constant record/indirect-jump route is
the reusable authority and that only ordinary store printing/value-use
admission is missing.

Actions:

- inspect `e8a0f70b4`/`6803f8c25`, `LirStoreOp` rendering, and function
  value-use verification
- name the exact legal store value representation and malformed boundary
- do not alter the first accepted consumer route

Completion check:

- the repair is bounded to direct-constant store consumption and preserves the
  original native representation/indirect-jump contract.

Completed: lifecycle diagnosis established this boundary; no source, test, or
proof artifact changed in 768 Step 5.

### Step 2 - Implement direct-constant ordinary pointer-store consumption

Goal: make the structured native direct constant legal for `LirStoreOp.val`
and validation without treating it as an instruction definition.

Actions:

- admit `LirOperandKind::DirectConstant` only where the ordinary store value
  grammar permits this direct pointer constant and render it via the structured
  direct-constant resolver
- extend function value-use/owned store checks to recognize the matching valid
  function-owned direct constant as a definition authority
- preserve pointer type, enclosing function, target label, and produced
  `LirValueId` identity; reject malformed ownership, target, value, pointer
  type, and mismatched direct use
- touch no producer or downstream carrier/integration code

Completion check:

- a pointer store prints a legal direct `blockaddress(...)` value and verifier
  admission is limited to the matching structured direct constant.

### Step 3 - Prove the repaired contract and return to 768

Goal: prove the ordinary store consumer route while retaining the accepted
indirect-jump route, then restore the interrupted producer packet.

Actions:

- add focused positive store-consumer coverage plus nearby malformed cases for
  invalid/missing/foreign owner, target, or value; non-pointer/wrong type; and
  mismatched direct use
- run a fresh build and selected focused proof for the native contract
- record accepted proof and return 768 exactly to Step 5; do not claim 768
  producer recovery or external integration

Completion check:

- focused proof passes, malformed forms reject at the owned boundary, and 768
  can retry Step 5 without redoing Steps 1--4.
