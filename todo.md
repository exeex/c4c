# Current Packet

Status: Active
Source Idea Path: ideas/open/823_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and select one distinct native parameter-use row
你該做code review了

## Just Finished

- Parent 734 Step 7.35 accepted in `18443fc0f`: only the closed-818
  DirectScalar `LirBinOp.lhs` authority was received into the typed Raw-BIR
  binary destination. Focused proof passed 1/1; matching full CTest guard was
  non-regressive at 3003/3038 pass and 35 failures before/after.

## Suggested Next

- Step 1 only: trace and select one distinct native function-body parameter
  use with a complete structured authority tuple. Do not select from text or
  begin Raw-BIR receiver work.

## Watchouts

- Ideas 821 and 822 retain pending, unaccepted implementation work; do not
  modify, discard, or claim acceptance for either slice.
- No generic scalar/parameter admission, Raw-BIR/importer/builder work, or
  presentation-derived recovery is authorized.
- The accepted pointer and DirectScalar binary-LHS rows are historical
  progress; do not reopen or repeat them.

## Proof

- Parent acceptance: fresh `^backend_lir_selected_pointer_authority$` 1/1;
  matching full CTest `test_before.log`/`test_after.log` guard, 3003/3038 pass
  and 35 failures before/after, non-regressive; receiver commit `18443fc0f`.
- This successor must select a fresh focused producer proof after Step 1.
