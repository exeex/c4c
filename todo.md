Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Broader Backend Checkpoint

# Current Packet

## Just Finished

Step 4 broader backend checkpoint passed after the call move boundary
consolidation checkpoint. The exact command completed successfully and
`test_after.log` records 162 backend tests passing with no failures.

No blocker remains for this checkpoint.

## Suggested Next

Next packet should route to plan-owner for Step 5 closure review or any
lifecycle decision the supervisor wants before closing/deactivating this active
plan.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into whole dispatch cleanup or a whole
  calls-family rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- Keep AArch64-specific register, memory, frame-slot, byval lane, immediate,
  constant-materialization, and assembly spelling in AArch64 code.
- Keep source-idea progress tied to deleted duplication, moved ownership, or a
  sharper emission-only boundary.
- Do not revisit completed call printing/effect-publication routes unless this
  mapping proves one still owns duplicate prepared-fact authority.
- The old mixed `make_prior_preserved_call_argument_source` overload body was
  removed after confirming the active declaration and call site use the
  narrowed helper that consumes `PreparedCallPreservedValue`.
- Missing prepared-fact blocker remains: there is no single prepared
  call-argument source fact that says a selected BeforeCall argument move should
  source from prior preservation versus local-frame address materialization
  versus byval register-lane materialization. Caller-side source selection is
  therefore still intentionally present.
- Do not use this target to refactor `lower_value_moves` wholesale, merge
  BlockEntry republication with BeforeInstruction lowering, or change move
  ordering.
- `find_prior_stack_preserved_value_before_instruction` still owns prior
  stack-preservation lookup and stack-route eligibility. The new
  `make_prior_stack_preserved_value_source` helper should stay operand
  construction only.
- `make_callee_saved_preservation_home_population` should stay an emission
  helper for already-eligible prepared effects; prior-preserved suppression now
  belongs in the `lower_before_call_moves` population-effect loop.
- Remaining source selection inside `lower_before_call_move` is tied to the
  missing prepared call-argument source fact noted above, so do not treat it as
  another narrow Step 2 target unless that prepared fact exists.
- Do not remove the remaining calls move/preservation declarations from
  `calls.hpp` without first moving their definitions or callers into the same
  translation unit; the current remaining declarations all have cross-TU uses.

## Proof

Proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Backend result: `test_after.log` records 162/162 tests passing.
