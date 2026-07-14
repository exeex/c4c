# Current Packet

Status: Active
Source Idea Path: ideas/open/767_lir_computed_goto_table_element_pointer_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Inventory table-element producer and result seams

## Just Finished

- Completed 767 Step 1 baseline: `comp-goto-1` passes (1/5), while
  `20040302-1`, `20041214-1`, `920501-4`, and `920501-5` fail (4/5) at the
  same missing `LirIndirectBrOp.addr_value` authority check. The split confirms
  the remaining family is table-element source forms, not generic indirect-
  branch publication.

## Suggested Next

- Inventory only the static-local versus local table-element source forms and
  their direct producer/result seams. Do not repair, add probes, or change
  publication in this packet.

## Watchouts

- The statement seam already copies `addr.value_id()`: do not patch
  `IndirBrStmt`/`addr_value`, weaken a verifier, or derive authority from text.
- Use frontend-LIR direct production tests; backend/case and the four external
  cases are integration proof only. Do not touch Raw-BIR/importer, 734, or
  accepted 765/766 work.

## Proof

- Step 1 ran fresh `cmake --build --preset default` successfully, then exactly
  `ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_comp_goto_1_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20041214_1_c|llvm_gcc_c_torture_src_920501_4_c|llvm_gcc_c_torture_src_920501_5_c)$'`;
  `comp-goto-1` passed and the remaining four failed at the same missing
  carrier authority. No canonical test logs were overwritten.
