# RV64 Object Route Scalar Compare Trunc Lowering

Status: Closed
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/372_rv64_object_route_frame_slot_address_call_args.md`

## Goal

Lower prepared scalar compare-result fragments that feed integer truncation in
the RV64 object route.

## Why This Exists

Idea 372 closed the frame-slot address call-argument route after focused support
for `LocalFrameAddressMaterialization` GPR call arguments landed. Its
representative rerun for `src/20000217-1.c` advanced to a distinct residual in
`showbug`:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

Packet probes narrowed the residual to scalar compare-result lowering where
`sge` produces a boolean result that is then represented as `trunc i1 -> i16`
in semantic BIR and as a prepared integer truncation of the compare result.
Existing object-route scalar compare support covers narrower `eq`/`ne` shapes,
so this is not frame-slot address publication work.

Representative evidence:

- `src/20000217-1.c`: `showbug` contains `bir.sge i32 %t8, 8` feeding
  `bir.trunc i1 %t9 to i16`, captured in
  `build/agent_state/372_step2_20000217-1.showbug-semantic-bir.txt` and
  `build/agent_state/372_step2_20000217-1.showbug-prepared-bir.txt`.

## In Scope

- Audit prepared compare and trunc metadata for the `sge` plus trunc residual.
- Define the first supportable RV64 object-route compare-result publication
  form using explicit prepared value homes, widths, and destination facts.
- Implement semantic lowering for the supportable scalar compare/trunc form.
- Preserve existing `eq`/`ne` compare-result behavior.
- Keep unsupported compare predicates, widths, dynamic or missing value homes,
  non-integer destinations, and ambiguous trunc sources fail-closed with
  precise diagnostics.
- Add focused backend tests for admitted and rejected compare/trunc forms.
- Rerun `src/20000217-1.c` and record whether it passes or advances to the next
  out-of-scope owner.

## Out of Scope

- Frame-slot address call-argument materialization; idea 372 closed that route.
- Pointer-value local-memory loads/stores; idea 368 closed that route.
- Frame-slot payload-value call arguments; idea 373 closed that route.
- Non-register parameter homes; use
  `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- Aggregate `va_arg`, byval aggregate homes, terminator lowering, or CFG
  reconstruction.
- Inferring compare semantics from source syntax or testcase names.

## Acceptance Criteria

- Focused RV64 object-emission tests prove the selected scalar compare/trunc
  lowering path.
- Unsupported adjacent compare/trunc shapes keep precise diagnostics.
- `src/20000217-1.c` is rerun and either passes or advances to a documented
  next owner because of semantic compare/trunc repair.
- Existing focused backend object-emission and prepared-contract coverage
  remains green.

## Completion Notes

Closed by the active Step 4 lifecycle decision after the implementation and
evidence commits:

- `26b76241` lowered the first supportable prepared scalar `Sge i32` compare
  result feeding an `i32 -> i16` trunc publication, using prepared metadata
  instead of testcase-specific constants or homes.
- `eb6ef452` recorded the representative rerun for
  `src/20000217-1.c`.
- Step 1 audit evidence is in
  `build/agent_state/375_step1_scalar_compare_trunc_audit.txt`.
- Step 3 rerun evidence is in
  `build/agent_state/375_step3_20000217-1.runner.log` and
  `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`.

The representative advanced from the scalar compare/trunc blocker to the
separate residual:

```text
unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves
```

That residual is now owned by:

- `ideas/open/376_rv64_object_route_prepared_move_bundle_target_shapes.md`

Close-time regression gate used the focused backend coverage from this idea:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test &&
ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed with no new failures and no pass-count regression.

## Reviewer Reject Signals

- Reject testcase-name shortcuts for `src/20000217-1.c` or its exact source
  expression shape.
- Reject hard-coded compare predicates, constants, value ids, frame slots, or
  destination registers not derived from prepared metadata.
- Reject changes that only rename diagnostics or rewrite expectations while
  leaving the same compare/trunc lowering unsupported.
- Reject broad terminator, call-argument, parameter-home, or local-memory
  rewrites inside this idea.
- Reject expectation downgrades, skip broadening, allowlist filtering, or
  unsupported-test weakening claimed as capability progress.
