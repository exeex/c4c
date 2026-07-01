Status: Active
Source Idea Path: ideas/open/512_stack_passed_parameter_home_publication.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Validate And Handoff

# Current Packet

## Just Finished

Step 8 of `plan.md` completed validation and handoff for the stack-passed
parameter-home publication plan.

Validated outcome:

- Producer/lowering now publishes ordinary-C stack call/formal ABI metadata
  for fixed scalar values beyond the current RV64 argument register budget.
- Prealloc now publishes prepared stack homes for authorized stack-passed
  formals that lacked a unique prior spill-slot home.
- RV64 object admission now consumes prepared stack-passed scalar formal homes
  directly from explicit `param.abi->passed_on_stack` and frame-slot facts.
- Focused coverage proves accepted I64 and F64 stack-passed formal homes and
  fail-closed malformed offsets, frame slots, source kinds, missing
  `passed_on_stack`, memory class, and F128.
- `20001017-1.c` and the three remaining rows no longer expose the old
  `unsupported_param_home` gap when prepared homes are coherent.

## Suggested Next

Plan owner should close idea 512 as complete for the stack-passed
parameter-home contract and, if desired, create a separate follow-up idea for
RV64 generic stack-to-stack prepared move-bundle materialization.

## Watchouts

- `20001017-1.c` remains blocked by the existing `fpr:fs1` callee-saved
  stack-frame diagnostic, not by the stack-passed parameter-home consumer.
- `20010518-1.c`, `pr27073.c`, and `pr69447.c` are no longer parameter-home
  rows at current `HEAD`; their remaining blocker is
  `unsupported_move_bundle_target_shape` for stack-to-stack value moves.
- Follow-up idea candidate: RV64 prepared move-bundle materialization for
  generic stack-to-stack value moves, with care not to fold it into the
  stack-parameter-home source idea.
- Keep varargs, F128, memory aggregate, dynamic stack, and unrelated RV64
  instruction-lowering gaps out of this source idea unless the supervisor or
  plan owner creates a separate follow-up.

## Proof

- `ctest --test-dir build -j --output-on-failure -R 'stack_passed_parameter_home|prepare_frame_stack_call_contract|prealloc_formal_publications|backend_riscv_object_emission|riscv64_byval|rv64_runtime_riscv64_byval|riscv64_aggregate'`
  passed, 22/22.
- `ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`
  passed, 345/345. `test_after.log` is the canonical after log for this
  validation packet.
- `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
  passed: before 331/331, after 345/345, no new failures or slow tests.
