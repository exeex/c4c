Status: Active
Source Idea Path: ideas/open/08_calls_argument_sources_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Move Or Redirect Remaining Source Choice To Prepared Facts

# Current Packet

## Just Finished

Step 2 retired the absent-selection local-frame-address stack-object recovery
path from `make_local_frame_address_call_argument_source`.

Prepared call planning now publishes a
`LocalFrameAddressMaterialization` source-selection fact for register-homed
local aggregate address arguments by selecting the prepared frame-slot
materialization or the matching local-slot frame object in
`src/backend/prealloc/call_plans.cpp`.

Review `review/calls_argument_sources_step2_local_frame_address_review.md`
found no blocking issues for the local-frame-address slice and recommended
continuing the current Step 2 route.

The AArch64 helper now consumes that prepared selection and no longer scans
`stack_layout.objects`/`frame_slots` itself when no local-frame-address
selection is present. Focused tests assert the prepared fact and update the
manual AArch64 dispatch fixtures to carry the same fact explicitly.

## Suggested Next

Continue Step 2 with the next absent-selection frame-slot source-choice path,
likely the remaining frame-slot value/address compatibility branches in
`make_frame_slot_call_argument_source` or
`make_frame_slot_call_argument_address_source`.

## Watchouts

- Keep semantic source-choice authority in prepared call facts, not AArch64
  helper reshuffles.
- Do not edit `ideas/closed/`, `review/`, or non-canonical test logs as part of
  this execution route.
- Treat expectation weakening, duplicate prepared/AArch64 source selection, and
  helper-only moves as route failures.
- Do not fold `find_frame_slot_by_id` into call-specific code; it is a shared
  lookup used by byval aggregate and cast lowering.
- The delegated proof regex `^(backend_|...)$` only matched the two named
  c_tests because `backend_` is anchored as a complete alternative; touched
  backend tests were run as additional focused validation and appended to
  `test_after.log`.
- `make_frame_slot_call_argument_address_source` still owns the separate
  missing-local-aggregate frame-slot address compatibility path for stack-homed
  arguments; this slice only retired the register-homed local-frame-address
  stack-object recovery.

## Proof

Passed; proof log preserved at `test_after.log`.

Delegated command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$'
```

Result: build succeeded; the delegated regex matched and passed
`c_testsuite_aarch64_backend_src_00204_c` and
`c_testsuite_aarch64_backend_src_00216_c`.

Additional focused validation appended to `test_after.log`:

```sh
ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch)$'
```

Result: both touched backend tests passed.

`git diff --check` also passed.

Supervisor follow-up validation:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: build succeeded and all 162 backend tests passed. `test_after.log`
now contains this accepted supervisor proof.
