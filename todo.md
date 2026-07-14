# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Enumerate separated label-address table producer forms

## Just Finished

- Completed plan.md Step 1 integration evidence only: after a fresh build, the
  exact five-consumer subset retained the 1/5 versus 4/5 boundary.
  `llvm_gcc_c_torture_src_comp_goto_1_c` passed; `20040302_1`, `20041214_1`,
  `920501_4`, and `920501_5` all failed only because
  `LirIndirectBrOp.addr_value` lacks current-function pointer `LirValueId`
  authority. This assigns no carrier or testcase-specific ownership.

## Suggested Next

- Step 2: enumerate separated label-address table producer forms, naming each
  direct frontend-LIR producer, structured input/result candidate, and first
  missing-authority boundary without selecting a repair.

## Watchouts

- Preserve the five external cases as integration evidence only. The carrier,
  verifier, Raw-BIR/importer, 734, and accepted 765/766/767 work remain out of
  scope; use direct frontend-LIR production probes rather than backend/case or
  named-testcase derivatives unless new evidence moves ownership.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^(llvm_gcc_c_torture_src_comp_goto_1_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20041214_1_c|llvm_gcc_c_torture_src_920501_4_c|llvm_gcc_c_torture_src_920501_5_c)$'
  > test_after.log`. The build passed; CTest intentionally returned nonzero
  with the expected four failures. Proof log: `test_after.log`.
