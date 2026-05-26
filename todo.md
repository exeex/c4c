Status: Active
Source Idea Path: ideas/open/17_aarch64_absent_selection_fallback_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire Local Aggregate Address Fallbacks

# Current Packet

## Just Finished

Step 2: Retire Local Aggregate Address Fallbacks completed for the owned
AArch64 dispatch path. `calls_dispatch_bridge.cpp` now materializes local
aggregate address call arguments only through the argument's prepared
`LocalFrameAddressMaterialization` selected-source facts via
`make_selected_call_argument_source`; absent, wrong-kind, incomplete, or
unprintable selections fail closed with `MissingValueAuthority` diagnostics
instead of reconstructing from `local_aggregate_address_frame_offset`.
`backend_aarch64_instruction_dispatch_test.cpp` keeps the positive direct local
aggregate address publication coverage and adds an absent-selection rejection
check for direct dispatch.

## Suggested Next

Supervisor can review and commit this Step 2 slice, then delegate Step 3 to
retire the indirect byval lane absent-selection fallback family.

## Watchouts

- Do not edit the source idea for routine audit findings.
- Do not treat expectation downgrades or fallback renames as progress.
- Keep `review/` artifacts transient.
- The byval family still has both register and stack-lane consumers; retiring
  only one branch would leave `collect_byval_register_lane_stores` available as
  an alternate absent-selection path.
- `clang-format` is not installed in this environment; formatting was kept
  manual.

## Proof

Ran the delegated proof:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch)$"'`.
Result: passed. Proof log: `test_after.log`.
