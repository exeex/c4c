# Current Packet

Status: Complete
Source Idea Path: ideas/open/535_rv64_object_frame_stack_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Pure Frame And Stack Helpers

## Just Finished

Completed Step 2 by moving the mapped pure RV64 frame and stack helper behavior
into `prepared_frame_emit.cpp`/`prepared_frame_emit.hpp` while preserving
object-route compatibility wrappers in `object_emission.cpp`.

Moved under the prepared-frame boundary:

- fixed frame sizing and validation helpers
- prepared stack-slot home offset helpers
- GPR register-home lookup helpers
- basic encoded GPR stack load/store helpers
- simple stack pointer adjustment and direct primitive dependencies

The compatibility wrappers keep current object-route call sites and names
stable, while forwarding the moved behavior through prepared-frame-prefixed
helpers. Call-specific byval/sret publication, before-return bundles, local and
global memory helpers, FPR stack helpers, traversal, prepared instruction
dispatch, data-object assembly, relocation handling, and ELF writing stayed in
`object_emission.cpp`.

## Suggested Next

Execute Step 3 by probing the remaining wrapper/include/API surface after the
move. Remove only redundant wrappers or includes whose removal does not expose
call, memory, traversal, or object-module dependencies through the prepared
frame helper API; otherwise record why the wrapper boundary should stay parked.

## Watchouts

- `object_emission.cpp` still intentionally keeps wrappers for object-route
  readability and to avoid broad call-site churn; do not remove them unless the
  dependency boundary remains cleaner without them.
- The moved stack-offset helpers use `RiscvEncodedFragment` via forward
  declaration in the header and `object_emission.hpp` in the implementation.
- `clang-format` was not available in this environment, so formatting was kept
  manual.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_stack_passed_parameter_home_publication|obj_runtime_rv64_large_fixed_frame_slot_access|obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload)'`

Proof log: `test_after.log`.
