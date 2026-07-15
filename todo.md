# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.38
Current Step Title: Receive the 825-authorized DirectScalar switch-selector parameter authority row

## Just Finished

- Step 7.38: received only `LirSwitch.selector_parameter_authority` for the
  closed-825 DirectScalar switch-selector row. The importer now checks native
  value/owner/index/type/ABI/role plus exact selector and selector-type
  equality before mapping the selected parameter directly to the typed
  Raw-BIR `SwitchTerm` selector; nearby coverage proves positive receipt and
  missing, invalid, duplicate, foreign, index/type/ABI/role, and both
  consumer-incoherent rollback paths.

## Suggested Next

- Return the exhausted 734 runbook to plan-owner for the source completion
  gate; do not infer source-idea completion from this one receiver row.

## Watchouts

- Do not touch preserved dirty Idea 821/822 work or reopen the accepted 734
  DirectPointer, DirectScalar binary-LHS/RHS, and ReturnValue rows.
- Do not reuse binary-LHS authority, materialize an `add`, or derive facts from
  names, signatures, rendered operands, diagnostics, or `monostate`.
- This packet intentionally adds no new Raw-BIR authority carrier: the typed
  switch destination retains the selected function parameter as its selector.

## Proof

- Passed fresh `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` (6/6).
  Canonical regression logs were not changed; they remain supervisor-owned.
