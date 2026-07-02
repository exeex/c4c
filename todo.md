Status: Active
Source Idea Path: ideas/open/513_rv64_stack_to_stack_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate And Hand Off

# Current Packet

## Just Finished

Completed Step 6 from `plan.md`: validated and handed off the RV64
stack-to-stack prepared move materialization runbook.

Close-readiness validation recorded by the supervisor:

- `cmake --build build --target c4cll` passed with `ninja: no work to do`.
- `cmake --build build --target backend_riscv_object_emission_test` passed with
  `ninja: no work to do`.
- `ctest --test-dir build -j --output-on-failure -R 'backend_riscv_object_emission|stack_to_stack|move_bundle'`
  passed, 1/1.
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  passed, 345/345.
- `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
  passed: before 331/331, after 345/345, no new failures.

Runbook handoff state:

- Step 3 implemented coherent same-scalar stack-slot-to-stack-slot
  materialization from prepared move/source/destination facts.
- Step 4 confirmed focused accepted and reject coverage in
  `backend_riscv_object_emission_test`.
- Step 5 recorded that the three representative rows still fail at
  `unsupported_move_bundle_target_shape`, but their first failures are outside
  the accepted 513 shape: multi-move register-source bundle, register-source to
  stack mismatch, and `bank=none`/conversion-adjacent source storage.
- The source idea's stack-to-stack prepared move materialization scope is
  close-ready; remaining representative row failures should be split into
  follow-up owner ideas instead of expanding this runbook.

## Suggested Next

Ask the plan owner to close or deactivate this runbook and create any needed
follow-up ideas for the Step 5 row classifications.

## Watchouts

Do not expand idea 513 to infer stack locations, merge multi-move bundles, or
invent conversion semantics for the remaining representative rows. Their
continued diagnostic text overlaps the original gate, but Step 5 separated the
remaining first failures from the coherent same-scalar stack-slot
materialization owner.

The plan owner should treat plan exhaustion separately from source-idea
closure. If closure is accepted, follow-ups should preserve the no-overfit rule
and name semantic owners rather than row names.

## Proof

Used the delegated supervisor-provided close-readiness proof:

- `cmake --build build --target c4cll`: passed.
- `cmake --build build --target backend_riscv_object_emission_test`: passed.
- `ctest --test-dir build -j --output-on-failure -R 'backend_riscv_object_emission|stack_to_stack|move_bundle'`:
  passed, 1/1.
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`:
  passed, 345/345; proof log path is `test_after.log`.
- `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`:
  passed with before 331/331 and after 345/345.

This packet also ran `git diff --check -- todo.md` after the todo-only update.
