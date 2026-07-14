# Current Packet

Status: Active
Source Idea Path: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish and prove production computed-goto address carrier authority

## Just Finished

- 767 is capability-complete and closed after accepted Step 5 commit
  `403e86afd`: its generic table-element access route retains supported global
  and current-function-local base authority through typed GEP and exact pointer
  load results. Direct frontend-LIR proof and the aggregate `^frontend_cxx_`
  CTest bucket passed; it made no carrier-publication claim.

## Suggested Next

- Resume 764 Step 1: freshly rerun the preserved five consumers after 767,
  then publish `LirIndirectBrOp.addr_value` only if the shared carrier seam
  still lacks authority. Do not reimplement the accepted table-element route.

## Watchouts

- 767 is closed; treat its static/local table-element producer/result contract
  as an accepted prerequisite, not scope for 764. No Raw-BIR/importer or 734
  receiver work; no verifier weakening, text recovery, testcase branching, or
  broad pointer/table redesign.

## Proof

- Accepted 767 prerequisite: `403e86afd`; fresh build, direct
  `./build/tests/frontend/frontend_lir_call_type_ref_test`, exact aggregate
  `^frontend_cxx_` CTest 1/1, and its matching allow-non-decreasing guard
  passed. Existing before/after frontend CTest evidence remains recorded; no
  logs were changed during this lifecycle switch.
- Before handoff to 734, run a fresh build, focused carrier positive/malformed
  proof, and exactly `ctest --test-dir build -j --output-on-failure -R
  '^(llvm_gcc_c_torture_src_comp_goto_1_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20041214_1_c|llvm_gcc_c_torture_src_920501_4_c|llvm_gcc_c_torture_src_920501_5_c)$'`.
