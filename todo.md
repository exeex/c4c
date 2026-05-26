Status: Active
Source Idea Path: ideas/open/17_aarch64_absent_selection_fallback_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire Local Aggregate Address Fallbacks

# Current Packet

## Just Finished

Step 2: Retire Local Aggregate Address Fallbacks regression follow-up
completed. `call_plans.cpp` now publishes
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

Supervisor can review and commit this Step 2 regression-fix slice, then decide
whether Step 2 is complete or whether another targeted local aggregate address
audit packet is needed before moving to Step 3.

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
