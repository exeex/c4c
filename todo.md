# Current Packet

Status: Active
Source Idea Path: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish and prove production computed-goto address carrier authority

## Just Finished

- 768 Step 5 is accepted in `628b55b9`: native direct `LabelAddrExpr` rvalue
  production emits the typed current-function/target/pointer/produced-value
  constant and preserves it through the accepted 771 initializer seam. Its
  fresh focused frontend-LIR proof and `^frontend_lir_` guard passed 7/7.
  The hook full-suite candidate was rejected (3034/3034 to 3032/3037): four
  computed-goto integrations stop at missing `LirIndirectBrOp.addr_value`,
  which is 764's external carrier/integration route; `pr70460` instead stops
  at empty `LirGepOp.ptr` and is not within 764 or 768.

## Suggested Next

- Resume 764 Step 1. Freshly rerun its preserved five computed-goto consumers
  (the passing `comp-goto-1` control plus the four carrier failures), trace
  the first current authority-loss owner, and publish `addr_value` only from a
  verified pointer result. Do not change 768's accepted producer seam.

## Watchouts

- 764 owns only the four `addr_value` external integration failures. The
  `pr70460` empty-GEP-pointer failure requires a separate open blocker before
  any future full-suite candidate can be accepted; do not absorb it into the
  carrier route. No Raw-BIR/importer, 734, text recovery, testcase routing,
  verifier weakening, or broad pointer/table redesign.

## Proof

- Accepted 768 Step 5: `628b55b9`; fresh build, focused
  `^frontend_lir_label_address_rvalue_probe$`, and `^frontend_lir_` guard 7/7.
- 764 required proof: fresh build, focused carrier positive/malformed proof,
  and `ctest --test-dir build -j --output-on-failure -R
  '^(llvm_gcc_c_torture_src_comp_goto_1_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20041214_1_c|llvm_gcc_c_torture_src_920501_4_c|llvm_gcc_c_torture_src_920501_5_c)$'`.
