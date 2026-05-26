Status: Active
Source Idea Path: ideas/open/17_aarch64_absent_selection_fallback_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Retire Indirect Byval Lane Payload Fallbacks

# Current Packet

## Just Finished

Step 2: Retire Local Aggregate Address Fallbacks is complete after
`999fc953e` and `ba284bd3a`. `calls_dispatch_bridge.cpp` now consumes prepared
`LocalFrameAddressMaterialization` selected-source facts for the owned AArch64
dispatch route and fails closed on absent or incomplete selected source facts.
`call_plans.cpp` now publishes
`LocalFrameAddressMaterialization` selected-source facts for same-block pointer
add/sub derived local aggregate addresses, carrying the prepared byte delta into
`source_stack_offset_bytes` instead of relying on target-local reconstruction.
It also narrows the prepared `allows_local_aggregate_address_publication` flag
to arguments that actually selected `LocalFrameAddressMaterialization`, so
ordinary pointer call arguments keep using their scalar/global producer routes
instead of being rejected by the local-frame fail-closed path.
`backend_prepare_frame_stack_call_contract_test.cpp` now covers a derived local
frame address selection with a nonzero byte delta. The direct absent-selection
dispatch path remains fail-closed through the existing Step 2 diagnostics and
manual dispatch coverage.

## Suggested Next

Delegate Step 3: retire the indirect byval lane absent-selection fallback
family. Start from the Step 1 byval fallback map in the history of `todo.md`
and keep register-lane and stack-lane publication on complete prepared
`ByvalRegisterLane` selected-source facts rather than
`collect_byval_register_lane_stores` reconstruction.

## Watchouts

- Do not edit the source idea for routine audit findings.
- Do not treat expectation downgrades or fallback renames as progress.
- Keep `review/` artifacts transient.
- The byval family still has both register and stack-lane consumers; retiring
  only one branch would leave `collect_byval_register_lane_stores` available as
  an alternate absent-selection path.
- `clang-format` is not installed in this environment; formatting was kept
  manual.
- The parent of `999fc953e` passes the four delegated c-testsuite cases. The
  fixed regression was not only a bad derived local-frame offset; the broad
  publication flag also routed ordinary pointer call arguments through the
  local aggregate fail-closed gate.

## Proof

Ran the delegated proof:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(164|180|195|216)_c)$"'`.
Result: passed. Proof log: `test_after.log`.
