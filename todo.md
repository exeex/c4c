# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair and prove native direct LabelAddrExpr rvalue production

## Just Finished

- 770 completed the bounded native direct-constant dependency
  (`e8a0f70b4`; proof-state `6803f8c25`) with a fresh build and passing
  `^backend_lir_to_bir_interface$` proof. That contract preserves a direct
  label-address constant through Raw-BIR source-value and `IndirectJumpTerm`
  consumption. 768 Steps 1--4 remain accepted; its prior synthetic
  `LirLabelAddrOp` bridge remains rejected.

## Suggested Next

- Step 5: repair direct frontend-LIR `LabelAddrExpr` rvalue production using
  770's completed native direct-constant contract. Preserve typed current
  function, target-label, and produced-value authority and run the focused
  frontend-LIR positive/malformed proof. Do not touch the carrier, 767, 769,
  or automatic-table `DeclRef` decay.

## Watchouts

- No `select`, `gep`, `bitcast`, or other synthetic materialization is an
  acceptable label-address identity bridge. Do not add a carrier change,
  raw-text recovery, testcase-shaped routing, or reopen accepted 767/769
  contracts. The automatic local-table `DeclRef` decay-to-`LirGepOp`
  local-slot/two-index authority contract is not in this packet.

## Proof

- Accepted evidence retained for the rejected slice: fresh targeted build and
  focused `frontend_lir_label_address_rvalue_probe` proof passed; the matching
  regression guard passed with `allow-non-decreasing`; and
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_'` was 7/7
  green. None overrides the no-synthetic-bridge scope gate.
