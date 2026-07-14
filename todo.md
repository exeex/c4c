# Current Packet

Status: Active
Source Idea Path: ideas/open/750_lir_cfg_terminator_block_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish computed-goto target-list authority

## Just Finished

- Step 2 completed: `LirIndirectBrOp.targets` now has ordered verifier-checked
  current-function `LirBlockId` authority; labels remain checked display
  mirrors, with focused positive and missing, invalid, duplicate, ambiguous,
  foreign, and misleading-label coverage.

## Suggested Next

- Execute Step 3 only: consolidate active terminator successor verification and
  prepare the precise receiver handoff for the paused Raw-BIR source.

## Watchouts

- Labels are display-only. Step 3 must not absorb PHI, Raw-BIR receiver,
  local/object, memory/va, or aggregate/vector work.

## Proof

- Focused: `cmake --build --preset default --target frontend_lir_call_type_ref_test && ./build/tests/frontend/frontend_lir_call_type_ref_test` (pass).
- Accepted matching guard: pre/post `ctest --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'` (1/1 pass before and after; passed with `--allow-non-decreasing-passed`; post log: `test_after.log`).
