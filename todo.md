Status: Active
Source Idea Path: ideas/open/119_bir_block_label_structured_identity_for_assembler.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Align Prepared Backend Handoff

# Current Packet

## Just Finished

Step 5 prepared backend handoff completed in
`src/backend/prealloc/legalize.cpp` and
`tests/backend/backend_prepare_structured_context_test.cpp`.

Prepared control-flow publication now routes block labels through a local
legalize helper that resolves a BIR `BlockLabelId` with
`bir::Module::names.block_labels` and interns that spelling into
`PreparedNameTables::block_labels`, falling back to the raw string when the id
is absent or unresolvable. `PreparedControlFlowBlock` and
`PreparedBranchCondition` now use that helper for block, branch target, and
conditional true/false labels.

Added direct prepared-control-flow tests proving structured-id authority when
raw strings are deliberately drifted and raw-string fallback behavior when ids
are absent.

## Suggested Next

Next coherent packet: supervisor/plan-owner should decide whether Step 5
exhausts the active runbook or whether another downstream label consumer still
belongs in this plan before closure.

## Watchouts

- Existing prepared backend subsets stayed green under the delegated proof.
- Keep raw label strings as fallbacks until a later cleanup idea removes them.
- Do not expand this plan into MIR target codegen migration.
- Same-spelled labels such as `entry` across functions will share an id under
  the module table, which matches the prepared backend spelling-table model.
- Later prepared phases may still intern raw spellings for separate legacy
  dependencies; this packet only moved the legalize-published prepared
  control-flow contract.

## Proof

Passed. Proof log: `test_after.log`.

Command run exactly:
`( cmake --build build-backend --target c4cll backend_prepare_structured_context_test backend_prepare_phi_materialize_test backend_prepare_liveness_test backend_prepare_stack_layout_test && ctest --test-dir build-backend/tests/backend -R 'backend_prepare_(structured_context|phi_materialize|liveness|stack_layout)$' --output-on-failure ) > test_after.log 2>&1`

Result: `c4cll`, `backend_prepare_structured_context_test`,
`backend_prepare_phi_materialize_test`, `backend_prepare_liveness_test`, and
`backend_prepare_stack_layout_test` built successfully and all 4 selected
backend tests passed.
