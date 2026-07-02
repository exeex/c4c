# Current Packet

Status: Complete
Source Idea Path: ideas/open/535_rv64_object_frame_stack_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Probe Remaining Frame Cleanup

## Just Finished

Completed Step 3 by probing the remaining object-route wrapper/include/API
surface after the RV64 frame stack helper move.

Removed unused fixed-frame compatibility wrappers from `object_emission.cpp`
that only forwarded to prepared-frame helpers:

- `rv64_supported_fixed_frame_alignment`
- `align_rv64_object_stack_frame_size`
- `rv64_find_function_frame_slot`
- `rv64_frame_slot_extent_is_supported`
- `rv64_validated_prepared_fixed_frame_size`

Left the high-traffic object-route helper wrappers parked because their call
sites still cross call-specific publication, memory, traversal, FPR-adjacent,
and prepared instruction dispatch code. Replacing those local object-route
names with prepared-frame API names would be broad churn rather than a cleaner
boundary.

## Suggested Next

Execute Step 4 close-readiness review: compare the moved prepared-frame helper
surface against the source idea, confirm the remaining parked wrappers are
intentional, and decide whether this runbook is ready for supervisor review or
needs a follow-up split.

## Watchouts

- The remaining object-route stack/register wrappers are not dead code; they
  have many call sites across object-specific lowering paths.
- `prepared_frame_emit.hpp` still exposes the moved safe helper set used by
  other prepared emitters; no include cleanup was safe inside this packet.
- `clang-format` was not available in this environment, so formatting remained
  manual.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_stack_passed_parameter_home_publication|obj_runtime_rv64_large_fixed_frame_slot_access|obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload)'`

Proof log: `test_after.log`.
