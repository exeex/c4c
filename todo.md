# Current Packet

Status: Active
Source Idea Path: ideas/open/767_lir_computed_goto_table_element_pointer_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the table-element failure-family baseline

## Just Finished

- Switched from 764 Step 1 after 765 and 766 resolved the arithmetic route but
  the fresh preserved five-case proof reached only 1/5 passing. The remaining
  four failures share missing `LirIndirectBrOp.addr_value` authority and are
  table-element source forms, not a generic `IndirBrStmt` publication defect.

## Suggested Next

- Reproduce the exact five-case baseline, preserve the 1/5 versus 4/5 split,
  then inventory static-local and local table-element load/result producers
  before creating any focused frontend-LIR capability probe.

## Watchouts

- The statement seam already copies `addr.value_id()`: do not patch
  `IndirBrStmt`/`addr_value`, weaken a verifier, or derive authority from text.
- Use frontend-LIR direct production tests; backend/case and the four external
  cases are integration proof only. Do not touch Raw-BIR/importer, 734, or
  accepted 765/766 work.

## Proof

- Incoming exact proof: fresh `cmake --build --preset default`, then
  `ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_comp_goto_1_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20041214_1_c|llvm_gcc_c_torture_src_920501_4_c|llvm_gcc_c_torture_src_920501_5_c)$'`;
  `comp-goto-1` passes and the remaining four fail at the same missing carrier
  authority.
- Step 1 repeats that build-plus-command baseline before source-form probes;
  later steps use directly relevant frontend-LIR production proof selected from
  the mapped producer/result contracts.
