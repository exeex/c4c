Status: Active
Source Idea Path: ideas/open/07_aarch64_call_boundary_move_emission_only.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Redirect Source Consumption To Prepared Records

# Current Packet

## Just Finished

Step 3 implementation completed for AArch64 prepared source-kind consumption.

Redirected call-boundary source routing so explicit `FrameSlotAddress`,
`FrameSlotValue`, `LocalFrameAddressMaterialization`, and `ByvalRegisterLane`
selections drive the covered source paths through
`PreparedCallArgumentSourceSelection::kind` instead of probing legacy helpers
to rediscover the winning source. Explicit byval-lane selections now require
complete prepared selected source bytes; absent selections keep the existing
legacy compatibility scans.

## Suggested Next

Review whether the remaining call-boundary source paths should retire legacy
absent-selection compatibility once upstream prepared facts cover them.

## Watchouts

- Keep source-selection authority in prepared call facts when
  `source_selection` is present.
- Do not touch the transient `review/` artifacts unless explicitly delegated.
- Treat expectation weakening or named-test shortcuts as route failures.
- Byval payload-store discovery remains only for absent selections or existing
  non-Byval compatibility cases; explicit `ByvalRegisterLane` selections fail
  closed unless the selected source bytes are complete.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_prepared_memory_operand_records|backend_aarch64_operand_resolution)$'`

Passed. Proof log: `test_after.log`.

Supervisor follow-up validation:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed 162/162.
