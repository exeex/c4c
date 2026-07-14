# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair and prove native direct LabelAddrExpr rvalue production

## Just Finished

- 770 completed its bounded native direct-constant contract and its reopened
  ordinary pointer-store consumer repair: `e8a0f70b4`, `6803f8c25`, and
  `0a2778f0d`. A fresh build and passing
  `^backend_lir_to_bir_interface$` proof are retained in the matching root
  logs. 768 Steps 1--4 remain accepted; its prior synthetic `LirLabelAddrOp`
  bridge remains rejected.

## Suggested Next

- Step 5: repair direct frontend-LIR `LabelAddrExpr` rvalue production using
  770's completed native direct-constant contract, including its legal
  pointer-store consumer. Preserve typed current function, target-label, and
  produced-value authority and run the focused frontend-LIR positive/malformed
  proof. Do not touch the carrier, 767, 769, or automatic-table `DeclRef`
  decay.

## Watchouts

- No `select`, `gep`, `bitcast`, or other synthetic materialization is an
  acceptable label-address identity bridge. Do not add a carrier change,
  raw-text recovery, testcase-shaped routing, or reopen accepted 767/769
  contracts. The automatic local-table `DeclRef` decay-to-`LirGepOp`
  local-slot/two-index authority contract is not in this packet.

## Proof

- 770 acceptance evidence: fresh `cmake --build --preset default`, then
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`,
  passing and retained in `test_before.log` and `test_after.log`. It proves
  the native direct-constant consumer contract, not Step 5 producer recovery
  or external integration.
