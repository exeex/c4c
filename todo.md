# Current Packet

Status: Active
Source Idea Path: ideas/open/119_aarch64_calls_before_call_move_bundle_local_owner.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Neighboring Before-Call Routes

## Just Finished

Step 4 completed: proved the neighboring before-call routes after the in-place
`BeforeCallMoveLocalOwner::instruction` extraction.

Changed files:

- `todo.md`

No implementation file, expectation file, source idea, plan, or build metadata
was changed.

The delegated proof subset covers the neighboring before-call route families:

- Register arguments: `backend_aarch64_call_boundary_owner` directly lowers
  prepared before-call moves, and `backend_aarch64_instruction_dispatch` covers
  byval register-lane publication and direct/local aggregate address argument
  register materialization.
- Stack arguments: `backend_aarch64_instruction_dispatch` covers prepared
  frame-slot stack call arguments, semantic stack call argument destination
  offsets, aggregate-address stack copies, and prior stack-home argument
  republication.
- Immediate scalar arguments: `backend_aarch64_return_lowering` covers direct
  call immediate arguments, and `backend_aarch64_instruction_dispatch` covers
  prepared immediate stack call arguments lowered before a direct call.
- FP/f128 routes: `backend_aarch64_return_lowering` covers FP immediate call
  arguments, while `backend_aarch64_call_boundary_owner`,
  `backend_aarch64_target_instruction_records`, and
  `backend_aarch64_instruction_dispatch` cover f128 HFA/q-register call
  boundary movement and f128 constant/frame-slot call arguments.
- Byval/local-frame-address routes: `backend_aarch64_instruction_dispatch`
  covers small byval aggregate register lanes, prepared payload-lane loads,
  register-home and stack-home byval payload slots, variadic byval lane
  overlap checks, overflow byval prepared stack lanes, and selected local frame
  address materialization without legacy lookup rediscovery.
- Outgoing-stack-area routes: `backend_prepare_frame_stack_call_contract` and
  `backend_prealloc_call_boundary_classification` cover prepared call stack
  argument area planning/classification, and `backend_aarch64_instruction_dispatch`
  covers semantic stack call arguments that use a distinct outgoing area base,
  including the `PreparedCallPlan::outgoing_stack_argument_area` path consumed
  by AArch64-local `x16` base emission.

No Step 4 route coverage gap was found in the delegated focused subset.

## Suggested Next

Execute Step 5 acceptance review. Compare the final diff against idea 119's
owner-boundary criteria and confirm there is no stack-home reselecting, f128
carrier rediscovery, immediate-publication rediscovery, or outgoing-stack-area
recomputation before lifecycle closure is considered.

## Watchouts

- Step 4 did not add tests; it validated the existing focused backend/AArch64
  contract subset selected by the supervisor.
- The proof confirms route coverage, not lifecycle closure. Step 5 still needs
  the explicit acceptance review against the source idea's reject signals.

## Proof

Ran exactly:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_prepare_frame_stack_call_contract|backend_prealloc_call_boundary_classification|backend_aarch64_target_instruction_records|backend_aarch64_call_boundary_owner)$' >> test_after.log 2>&1
```

Result: passed, 6/6 tests. The build reported no work to do, then all selected
CTest cases passed.

Proof log path: `test_after.log`

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 6/6 before and 6/6 after, no new failures.
