# Current Packet

Status: Active
Source Idea Path: ideas/open/750_lir_cfg_terminator_block_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define and verify conditional/switch successor authority

## Just Finished

- Step 1 completed: `LirCondBr` true/false and `LirSwitch` default/case edges
  now carry verifier-checked current-function `LirBlockId` authority; labels
  remain checked display mirrors, with focused positive and misleading-label,
  missing, ambiguous, and foreign-authority coverage.

## Suggested Next

- Execute Step 2 only: publish verifier-checked ordered current-function block
  authority for `LirIndirectBrOp.targets` without parsing target labels.

## Watchouts

- Labels are display-only. Step 2 must remain limited to computed-goto target
  lists; do not absorb PHI, Raw-BIR receiver, local/object, memory/va, or
  aggregate/vector work.

## Proof

- Focused: `cmake --build --preset default --target frontend_lir_call_type_ref_test && ./build/tests/frontend/frontend_lir_call_type_ref_test` (pass).
- Accepted matching guard: pre/post `ctest --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'` (1/1 pass before and after; passed with `--allow-non-decreasing-passed`).
