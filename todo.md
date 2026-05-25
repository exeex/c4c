Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Next Surviving Boundary Leak

# Current Packet

## Just Finished

Step 1 - Select The Next Surviving Boundary Leak: selected the
`calls_dispatch_bridge.cpp` frame-slot call argument materialization bridge as
the next checkpoint target.

Selected boundary leak:

- `materialize_missing_frame_slot_call_arguments` walks
  `PreparedCallPlan::arguments`, then calls `find_bir_value_for_prepared_name`
  to recover a retained BIR value by prepared value name and emits a synthetic
  publication into the prepared call ABI register.
- That makes the AArch64 dispatch/calls bridge rederive value-source and
  publication authority from retained BIR after prepared call planning and
  prepared move bundles have already classified the call argument as a
  frame-slot source.
- The intended owner is the prepared call-boundary move/argument-source
  surface: Step 2 should remove or narrow this bridge by consuming existing
  `PreparedCallArgumentPlan`, `PreparedMoveBundle`, and `PreparedValueHome`
  facts through `calls_moves.cpp` / `calls_argument_sources.cpp`; if those
  facts cannot describe the needed source publication, stop and record the
  missing prepared fact instead of keeping the BIR-name scan.

## Suggested Next

Execute Step 2 - Remove Or Narrow The Selected Boundary.

Remove or narrow `materialize_missing_frame_slot_call_arguments` and its
dependency on `find_bir_value_for_prepared_name` for frame-slot call arguments.
Prefer routing the needed before-call publication through prepared move and
argument-source helpers. If Step 2 proves there is no prepared fact that
identifies the source value/publication input without retained-BIR recovery,
record that missing prepared authority as the blocker.

## Watchouts

- Do not touch `ideas/open/03_dispatch_responsibility_reduction.md`.
- Keep any dispatch work limited to AArch64 call-emission bridge boundaries.
- Do not invent a new shared prepared-call API unless the selected boundary
  proves a required prepared fact is missing.
- Do not broaden into `materialize_indirect_call_callee_to_prepared_register`
  or local aggregate address publication during this Step 2 packet.
- `find_bir_value_for_prepared_name` is also used by
  `materialize_call_boundary_source_to_destination`; do not delete the shared
  helper unless Step 2 removes all current call sites.
- Do not weaken unsupported or expected-output contracts.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Proof

Mapping-only Step 1 packet; no build or test command was required or run.

Focused Step 2 proof scope:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_call_boundary_effect_plan)$') > test_after.log 2>&1`

Escalate to `^backend_` if the implementation changes shared prepared lookup,
move-bundle, or call-boundary effect behavior beyond this AArch64 frame-slot
argument bridge.
