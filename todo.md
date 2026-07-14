# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair and prove native direct LabelAddrExpr rvalue production

## Just Finished

- Step 5's uncommitted `LirLabelAddrOp` slice is rejected and must not be
  accepted or handed off. Its printer represents the new result as
  `select i1 true, blockaddress(...), blockaddress(...)`, a synthetic identity
  bridge prohibited by 768. The accepted Step 4 seam selection remains direct
  frontend-LIR `LabelAddrExpr` rvalue production.

## Suggested Next

- Return to Step 5: replace the synthetic bridge with a native direct
  label-address value representation/lowering while preserving typed current
  function, target-label, and produced-value authority and its focused
  positive/malformed contract. Do not touch the carrier, 767, 769, or
  automatic-table `DeclRef` decay.

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
