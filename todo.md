# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the blocked label-address table failure-family baseline

## Just Finished

- Switched from interrupted 764 Step 1 after the accepted 765/766/767 producer
  prerequisites still left the same four table-family integrations without a
  target operand ID. The direct carrier seam copies `addr.value_id()` exactly,
  so 768 owns decomposition of the upstream label-address table
  initialization/representation and rvalue-consumption producer seams.

## Suggested Next

- Step 1: run the exact preserved five-consumer command after a fresh build
  and retain the 1/5 versus 4/5 boundary as integration evidence. Do not patch
  the carrier or implementation in this baseline packet.

## Watchouts

- The carrier, verifier, Raw-BIR/importer, 734, and accepted 765/766/767 work
  are out of scope. Use direct frontend-LIR production probes, not backend/case
  or named-testcase derivatives, unless evidence later moves ownership.

## Proof

- Accepted prerequisites: 765 `1e24e2081`, 766 `74379f4a2`, and 767
  `403e86afd`; 767's fresh build, direct frontend-LIR test, aggregate
  `^frontend_cxx_` CTest 1/1, and matching guard passed. No logs change in
  this lifecycle switch.
- Step 1 proof is a fresh build followed by exactly `ctest --test-dir build -j
  --output-on-failure -R
  '^(llvm_gcc_c_torture_src_comp_goto_1_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20041214_1_c|llvm_gcc_c_torture_src_920501_4_c|llvm_gcc_c_torture_src_920501_5_c)$'`.
