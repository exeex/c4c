# Current Packet

Status: Active
Source Idea Path: ideas/open/810_lir_gep_producer_result_authority_baseline_blocker.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and classify failing GEP producer families

## Just Finished

810 Step 1 trace-only: reproduced `c_testsuite_src_00173_c` and traced its
authoritative GEP failure to pointer postfix increment lowering
(`emit_load_assignable_value` `fresh_value` base ->
`emit_assignable_incdec_value` `fresh_tmp` GEP result).  Evidence is in
`review/810_step1_gep_producer_trace.md`; no repair was made.

## Suggested Next

Select one bounded native pointer increment/decrement (and, if in scope,
pointer compound-add/sub) result-authority repair packet; do not group other
partial-log GEP failures without an independent producer trace.

## Watchouts

The partial root full-baseline log is diagnostic only, not a regression guard.
PHI residual failures belong to 804/806.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_src_00173_c$'` — build was up to date; the one-test subset failed as reproduced with `LirGepOp.result: authoritative GEP requires LirValueId result authority`.  The command does not write a root proof log; existing partial `test_after.log` was intentionally left untouched by this trace-only packet.
