# Current Packet

Status: Active
Source Idea Path: ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair the selected cast-result handoff

## Just Finished

796 Step 1 traced the 22 baseline `LirCastOp.result` raw-text residuals to the
single shared compatibility producer seam: `StmtEmitter::coerce` in
`src/codegen/lir/hir_to_lir/core.cpp`. Its scalar/vector/float/pointer cast
arms create `fresh_tmp()` string results before `emit_lir_op`; in contrast,
`StmtEmitter::coerce_operand` already uses `fresh_value()` for its selected
scalar native shapes and falls back to `coerce` for the residual compatibility
shapes. The verifier rejects the former seam's raw result at
`LirCastOp.result`; no rendered-text recovery is selected. The nonmatching
`frontend_lir_call_type_ref` and `20060910-1.c` residuals remain with existing
801 and 806 ideas, respectively.

## Suggested Next

796 Step 2: make only the selected `StmtEmitter::coerce` cast-result handoff
native, with nearby same-family positive and malformed-authority coverage.

## Watchouts

Do not touch the preserved dirty 801 aggregate/direct-complex hunks, 810/795
accepted GEP/parameter work, or the 806 PHI route. Keep unrelated raw
compatibility producers fail-closed; do not parse `%t` text or weaken the cast
verifier.

## Proof

Trace evidence: `StmtEmitter::coerce` / `StmtEmitter::coerce_operand` and
`verify_cast_op_authority`; no representative per-test command was retained
with the isolated 3013/3037 gate. Fresh delegated proof after this state update:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`.
Do not create or overwrite canonical root regression logs.
