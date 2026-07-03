Status: Active
Source Idea Path: ideas/open/565_prepared_move_bundle_widening_stack_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Prepared Widening Authority

# Current Packet

## Just Finished

Completed plan Step 3, `Repair Prepared Widening Authority`, for
`ideas/open/565_prepared_move_bundle_widening_stack_authority.md`.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Taught RV64 prepared object-route move-bundle materialization to consume
`PreparedMoveAuthorityKind::StackSlotWideningConversion` only for the explicit
stack-slot source to stack-slot destination widening shape. The materializer
loads the narrower source slot size into a scratch GPR and stores the widened
integer value to the destination stack slot size instead of treating the move
as a raw same-width stack byte copy.

Updated focused backend coverage so both representative width families,
`i8 -> i32` and `i16 -> i32`, build successfully and assert the narrow load
plus `i32` destination store sequence. Existing `authority=none`
stack-to-stack widening, explicit non-integer widening authority, and unrelated
unsupported shapes remain fail-closed.

Representative probe:

- `build/agent_state/565_step3_widening_materialization_after.allowlist`
- `build/agent_state/565_step3_widening_materialization_after.log`

Result: representatives still fail, `total=2 passed=0 failed=2`, but both rows
move past the old RV64 move-bundle materialization owner. Neither case log now
contains `unsupported_move_bundle_target_shape`,
`authority=stack_slot_widening_conversion`, or
`unsupported_prepared_move_bundle_classification`.

Downstream residuals after this slice:

- `src/20010224-1.c`: `[RV64_BACKEND_RUNTIME_MISMATCH]`,
  `clang_exit=0`, `c4c_exit=Segmentation fault`.
- `src/pr87623.c`: `[RV64_BACKEND_RUNTIME_MISMATCH]`, `clang_exit=0`,
  `c4c_exit=Subprocess aborted`.

The old `unsupported_move_bundle_target_shape` for
`stack_slot_widening_conversion` is gone. The remaining owner is downstream
runtime behavior for these representatives, not prepared move-bundle
classification or RV64 move-bundle materialization.

## Suggested Next

Plan Step 4, `Reconcile Representatives And Residual Owners`, is ready for
plan-owner or supervisor review. The next packet should decide whether the
active idea can close as the prepared/RV64 move-bundle authority repair, or
whether the runtime mismatches deserve a separate source idea.

## Watchouts

- The new RV64 materialization is scoped to explicit
  `StackSlotWideningConversion` authority on stack-slot to stack-slot moves.
  It must not be generalized into `authority=none` memory-to-memory copy
  authorization.
- The representative failures are now runtime mismatches. Do not conflate them
  with the completed prepared authority publication or RV64 move-bundle
  materialization repair without fresh evidence.
- Do not touch expectations, unsupported markers, allowlists, or pass/fail
  accounting.

## Proof

Ran the delegated proof command exactly:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: passed, `345/345` backend tests. Canonical proof log:
`test_after.log`.

Ran the required representative probe:

```sh
printf '%s\n' src/20010224-1.c src/pr87623.c > build/agent_state/565_step3_widening_materialization_after.allowlist && ALLOWLIST=build/agent_state/565_step3_widening_materialization_after.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/565_step3_widening_materialization_after.log 2>&1
```

Result: expected downstream failures remain, `total=2 passed=0 failed=2`, with
runtime mismatches recorded in
`build/agent_state/565_step3_widening_materialization_after.log`.
