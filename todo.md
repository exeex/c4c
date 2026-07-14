# Current Packet

Status: Active
Source Idea Path: ideas/open/770_lir_to_bir_native_label_address_constant_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement direct-constant ordinary pointer-store consumption

## Just Finished

- Lifecycle repair reopened 770 after 768 Step 5 found that its native direct
  constant cannot yet feed the ordinary `LirStoreOp` pointer initializer. 768
  made no code or test edits; its Steps 1--4 remain accepted and it will return
  to Step 5 after this bounded repair.

## Suggested Next

- Implement only legal `LirOperandKind::DirectConstant` pointer-store value
  rendering and matching function-owned direct-definition verification. Add
  focused positive/malformed store-consumer coverage, then build freshly and
  run the selected native-contract proof.

## Watchouts

- Preserve accepted `e8a0f70b4`/`6803f8c25` indirect-jump behavior. No
  `LabelAddrExpr` producer, carrier, table-decay, 767/769, broad backend, or
  synthetic `select`/`gep`/`bitcast` bridge work belongs here.

## Proof

- Required before return: fresh build plus focused native direct-constant
  positive/malformed proof covering ordinary pointer-store consumption and
  malformed owner/target/value/type/direct-use boundaries. Retain the accepted
  indirect-jump proof; do not claim 768 producer or external integration proof.
