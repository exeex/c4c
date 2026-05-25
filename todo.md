Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 5 closure review rejected closure for the register byval size
authority-removal checkpoint and kept
`ideas/open/02_aarch64_calls_emission_consolidation.md` open.

- The source idea remains incomplete because retained `CallInst::arg_abi` and
  `CallInst::arg_types` reads still decide publication and byval shape in
  target-local calls code.
- Surviving closure blockers:
  - `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`:
    `call_argument_allows_local_aggregate_address_publication` reads
    `CallInst::arg_abi` and `CallInst::arg_types` for aggregate address
    publication eligibility.
  - `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`:
    `call_argument_is_pointer`, `call_argument_is_byval_copy`, and
    `call_argument_allows_local_frame_address_publication` read
    `CallInst::arg_types` and `CallInst::arg_abi` for local frame address
    publication eligibility.
  - `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`:
    `aarch64_indirect_byval_argument_size_bytes`,
    `aarch64_stack_byval_argument_size_bytes`, and
    `aarch64_indirect_register_byval_argument` read `CallInst::arg_abi` for
    byval shape.
  - `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp` still exposes
    `CallInst`-shaped helper boundaries that must be retired or justified as
    emission-only after the publication path stops reconstructing call-plan
    decisions.
- `plan.md` was refreshed as the next residual authority checkpoint. The source
  idea was not edited or moved.

## Suggested Next

Route the next executor packet to Step 1: select one retained metadata
authority leak, map it to an existing prepared fact or a precise missing-fact
blocker, and record the focused proof command before implementation.

## Watchouts

- Do not close the source idea while any of the closure blockers above remain.
- Do not work from `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not replace retained reads with testcase-shaped shortcuts or weaker test
  expectations.
- A retained `bir::CallInst` read is acceptable only for instruction identity
  validation or diagnostics, not for deciding publication eligibility or byval
  shape already represented by prepared facts.

## Proof

No new build was required for this lifecycle-only closure review.

Accepted canonical proof artifact: `test_before.log`

Recorded command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_')`

Result: passed, `162/162` backend tests in `test_before.log`.

Close-time regression guard check for unchanged lifecycle review state:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed`

Result: passed.
