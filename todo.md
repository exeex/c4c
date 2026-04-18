Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished
- Plan Step: Step 2 - switched the bounded x86 compare-branch joined lane in
  `src/backend/mir/x86/codegen/prepared_module_emit.cpp` to take its branch and
  join ownership from `PreparedBirModule.control_flow` instead of reconstructing
  the lane from entry/select compare shape.
- Added joined-lane shared-contract coverage in
  `tests/backend/backend_prepare_phi_materialize_test.cpp` and a prepared-x86
  ownership check in `tests/backend/backend_x86_handoff_boundary_test.cpp` that
  desynchronizes the legalized compare/select carrier while keeping the
  authoritative control-flow metadata intact.
- Repaired the unrelated x86 short-circuit `or` guard continuation in
  `prepared_module_emit.cpp` so the true-return lane is duplicated canonically
  instead of being shared through an extra jump label.
- Tightened the LIR handoff boundary check so it proves
  `emit_target_lir_module` matches the canonical prepared-module consumer for
  the lowered semantic BIR instead of assuming every LIR fixture preserves a
  handwritten pre-lowering BIR asm shape.

## Suggested Next
- Plan Step: Step 2 / Step 3 - extend the prepared control-flow contract across
  one short-circuit-adjacent or loop-carried slice in shared lowering, then
  switch the matching x86 consumer off CFG-shape recovery.

## Watchouts
- Do not treat x86 matcher widening as progress for this idea.
- Keep semantic transfer ownership separate from idea `60` physical location
  ownership.
- The local-slot guard short-circuit lane still relies on x86-local detection
  logic outside the joined compare-branch slice; do not count that as shared
  contract progress.
- The LIR boundary helper now proves routing against the lowered prepared x86
  consumer, so exact asm-shape assertions for LIR fixtures belong in lowering
  or semantic-BIR tests, not this handoff seam.

## Proof
- Ran the delegated proof command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_phi_materialize|backend_x86_handoff_boundary(_test)?)$'`
- `test_after.log` captured the ctest portion of that proof run.
- Current result: delegated subset passed.
- Passing tests:
  `backend_prepare_phi_materialize`, `backend_x86_handoff_boundary`.
