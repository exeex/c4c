# Current Packet

Status: Active
Source Idea Path: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish and prove production computed-goto address carrier authority

## Just Finished

-  764 Step 1 repaired the `IndexExpr` coordinator conversion: non-vector
  indexed rvalues now return the typed loaded `LirOperand` directly instead of
  converting it through `emit_rval_payload` text and a raw operand. The
  computed-goto identity test now covers an indexed label-address table and
  rejects that route with a missing loaded-pointer value ID.

## Suggested Next

- Supervisor: review the completed Step 1 slice, compare its canonical
  regression evidence, and select the next active-plan packet without widening
  into `pr70460` or a general index/GEP redesign.

## Watchouts

- 764 owns only the four `addr_value` external integration failures. The
  `pr70460` empty-GEP-pointer failure requires a separate open blocker before
  any future full-suite candidate can be accepted; do not absorb it into the
  carrier route. No Raw-BIR/importer, 734, text recovery, testcase routing,
  verifier weakening, or broad pointer/table redesign.
- Vector indexing remains on its existing payload route; this packet changes
  only non-vector indexed rvalues that already materialize a typed access load.

## Proof

- 764 Step 1 passed fresh `cmake --build --preset default` and the delegated
  five-case computed-goto subset (5/5 passed):
  `ctest --test-dir build -j --output-on-failure -R
  '^(llvm_gcc_c_torture_src_comp_goto_1_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20041214_1_c|llvm_gcc_c_torture_src_920501_4_c|llvm_gcc_c_torture_src_920501_5_c)$' | tee test_after.log`.
  Focused `ctest --test-dir build --output-on-failure -R
  '^frontend_lir_call_type_ref$'` also passed. Proof log: `test_after.log`.
