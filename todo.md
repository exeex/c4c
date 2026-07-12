# Current Packet

Status: Active
Source Idea Path: ideas/open/721_x86_defined_function_prepared_core_completion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Strengthen Focused Contract Coverage

## Just Finished

- Step 3 added focused common producer/admission coverage proving both the
  repaired manual-phase grouped-spill definition and an ordinary fully
  legalized defined function produce genuine `PreparedMirCoreView` function
  views whose BIR, control-flow, addressing, and value-location facts retain one
  stable `FunctionNameId`.
- The same contract removes a required addressing bank and mismatches a required
  value-location bank from states that still claim legalization, proving both
  inconsistent preparations fail closed at common prepared-core admission.

## Suggested Next

- Step 4: run the planned broader validation and regression review for the
  completed prepared-core production slice.

## Watchouts

- The negative fixtures intentionally retain the producer's completed-phase
  claims and mutate only a required common prepared bank; broader validation
  should preserve that precise fail-closed distinction.
- No x86 fallback, emitter invariant, admission implementation, expectation
  classification, or production hook changed in Step 3.

## Proof

- Ran exactly:
  `cmake --build --preset default > test_after.log 2>&1; ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$' >> test_after.log 2>&1`.
- Build passed and the focused test passed (1/1). The supervisor-selected proof
  is sufficient for Step 3. Canonical log: `test_after.log`.
