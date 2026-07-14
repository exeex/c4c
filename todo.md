# Current Packet

Status: Active
Source Idea Path: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish and prove production computed-goto address carrier authority

## Just Finished

- 766 completed and was accepted in `74379f4a2`: the direct
  `emit_indexed_gep` operand overload now retains the verified SSA-based GEP
  pointer `LirValueId` in `LirGepOp.result`. Its focused proof and supervisor
  backend proof passed; it did not publish the downstream carrier field.

## Suggested Next

- Resume 764 Step 1: carry the accepted GEP pointer `LirValueId` through the
  direct `IndirBrStmt` seam into `LirIndirectBrOp.addr_value`, preserve all
  carrier verifier checks, then prove the shared five-consumer family.

## Watchouts

- Do not redo 766's GEP contract work, weaken a verifier, derive authority
  from text, or introduce a synthetic cast, alloca/load, phi, or select.
- Do not change Raw-BIR/importer or re-execute 734 Step 7.24. Treat
  `comp-goto-1`, `20040302-1`, `20041214-1`, `920501-4`, and `920501-5` as one
  family; none may be excluded, downgraded, or accepted as baseline debt.

## Proof

- Accepted prerequisite: `74379f4a2`; fresh `cmake --build --preset default`,
  focused `^frontend_lir_call_type_ref$`, a non-regressive matching guard, and
  supervisor backend proof 5/5 passed. The focused output is in
  `test_after.log`.
- Before handoff to 734, run a fresh build, focused positive/malformed carrier
  proof, and the preserved five-consumer command:
  `ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_comp_goto_1_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20041214_1_c|llvm_gcc_c_torture_src_920501_4_c|llvm_gcc_c_torture_src_920501_5_c)$'`.
