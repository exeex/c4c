# Current Packet

Status: Active
Source Idea Path: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and publish production computed-goto address authority

## Just Finished

- None; this blocker was activated after 734 Step 7.24 was accepted at the
  receiver boundary and broader validation exposed the separate producer gap.

## Suggested Next

- Reproduce the focused production failure and identify the first rvalue/operand
  owner that fails to carry the current-function pointer `LirValueId` to
  `LirIndirectBrOp.addr_value`.

## Watchouts

- Keep Raw-BIR/importer receiver code and the accepted 734 Step 7.24 packet
  untouched; resume 734 only after this producer handoff is accepted.
- Do not derive authority from `addr`, labels, rendered output, or testcase
  text, and do not claim the four unrelated full-suite failures.

## Proof

- Initial reproduction: `ctest --test-dir build -V -R '^llvm_gcc_c_torture_src_comp_goto_1_c$'`.
- Before handoff: fresh `cmake --build --preset default` plus the focused
  production-path proof selected during Step 1. The supervisor owns broader
  acceptance and canonical regression logs.
