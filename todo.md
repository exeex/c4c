# Current Packet

Status: Active
Source Idea Path: ideas/open/770_lir_to_bir_native_label_address_constant_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Specify the native direct-constant authority boundary

## Just Finished

- Switched from 768 Step 5 after its direct `LabelAddrExpr` synthetic
  `select` bridge was rejected. 768 preserves completed Steps 1--4 and the
  exact return point; this blocker owns only the missing native constant
  representation/lowering contract.

## Suggested Next

- Map the smallest LIR operand/value and LIR-to-BIR representation boundary
  that can carry direct `blockaddress` function/target/pointer/value identity
  through indirect-jump consumption without an instruction-shaped SSA bridge.

## Watchouts

- LLVM 19 does not accept `%x = blockaddress(...)` as an instruction. Do not
  use `select`, `gep`, `bitcast`, text recovery, or dummy results. Do not touch
  768 producer recovery, 764 carrier publication, external cases, table decay,
  767/769, Raw-BIR/importer, or broad backend work.

## Proof

- Lifecycle evidence: `a89f5f4c6` rejects the synthetic bridge; prior 768
  progress and accepted prerequisite references are preserved in its resumption
  record. Step 1 must select direct focused positive/malformed proof before
  implementation; no regression logs are changed by this switch.
