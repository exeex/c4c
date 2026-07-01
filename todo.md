Status: Active
Source Idea Path: ideas/open/444_frame_slot_call_argument_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Nearby Ordinary-C Coverage

# Current Packet

## Just Finished

Completed `plan.md` Step 4 focused ordinary-C coverage for the repaired
frame-slot address call-argument publication and RV64 object-consumption route.

Coverage added:

- New source case:
  `tests/backend/case/riscv64_frame_slot_pointer_arg_preserves_payload.c`.
- Registered required object-runtime test:
  `backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`.
- The case initializes two local pointer object slots from different
  address-of-local payloads, then passes both pointer-slot addresses as pointer
  arguments. The second pointer argument is a nonzero argument position.
- The callee writes distinct values through both pointer slots. If the second
  argument object slot is repopulated from stale state instead of the published
  `payload_frame_slot` route, the program returns the wrong value instead of
  the expected `7`.
- Fresh prepared-BIR evidence for the new case includes arg1
  `call_arg_value_publication` with `payload_frame_slot=#5`,
  `payload_stack_offset=4`, and `destination_stack_offset=16`.

Files changed:

- `tests/backend/case/riscv64_frame_slot_pointer_arg_preserves_payload.c`
- `tests/backend/CMakeLists.txt`

## Suggested Next

Execute Step 5 validation and handoff with a fresh representative object-route
proof, the new focused object-runtime coverage, and the supervisor-selected
backend subset.

## Watchouts

- `missing_frame_slot_arg_publication=yes` remains a need marker; it was not
  cosmetically suppressed.
- Text-route `--codegen asm` was probed for the new source. Emission succeeds,
  but the normal RV64 runtime route returned an illegal instruction under qemu,
  so the optional normal runtime CTest was intentionally left unregistered.
- No expectations, allowlists, unsupported markers, runtime comparison, or
  pass/fail accounting were changed.

## Proof

- Hook state aligned with
  `scripts/plan_review_state.py set-step --step-id 4 --step-title 'Add Nearby Ordinary-C Coverage'`.
- `cmake --build build --target c4cll` passed.
- Focused prepared-BIR evidence for the new source is under
  `build/agent_state/444_step4_coverage/frame_slot_pointer_arg.prepared-bir.txt`.
- `ctest --test-dir build -j --output-on-failure -R "backend_(obj_runtime_)?rv64_.*frame_slot_pointer_arg_preserves_payload"`
  passed.
- `git diff --check -- todo.md tests/backend/CMakeLists.txt tests/backend/case`
  passed.
- `ctest --test-dir build -j --output-on-failure -R "^backend_"` passed.
- `test_after.log` contains the combined proof output.
