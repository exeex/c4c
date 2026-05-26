Status: Active
Source Idea Path: ideas/open/09_calls_preservation_republication_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Prepare Preservation Source And Destination Facts

# Current Packet

## Just Finished

Step 2 - Prepare Preservation Source And Destination Facts completed the AArch64
endpoint-consumption regression repair.

Regression cause: commit 69d5214a4 made preservation home population consume
the prepared source endpoint before the legacy value home. Real c_testsuite
callee-saved preservation records can currently publish a source endpoint that
aliases the callee-saved destination register, for example `%p.i` in
`00168.c` publishing both source and destination as `x20` while the live value
home before the call is still `x0`. Treating that self-alias endpoint as
authoritative suppressed the needed `x0 -> x20` population move and corrupted
later uses after the call.

Repair: `src/backend/mir/aarch64/codegen/calls_moves.cpp` still consumes
prepared preservation source endpoints when they name a distinct register or a
prepared stack source, and still consumes prepared destination endpoints for
the callee-saved home. When a register source endpoint aliases the prepared
destination register, AArch64 now falls through to the existing value-home
source selection instead of treating the self-alias as a completed population.

`backend_aarch64_instruction_dispatch_test` now keeps the explicit endpoint
consumption test and adds coverage for the self-alias endpoint regression,
proving that a source endpoint equal to the callee-saved destination falls back
to the real value-home source (`x1 -> x20` in the fixture) without weakening
the distinct endpoint case.

Changed files:

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `todo.md`

## Suggested Next

Next Step 2 or Step 3 packet: decide whether the remaining callee-saved
self-alias source endpoint is a preparation-side publication gap to retire, or
move on to prepared post-call republication facts if the supervisor wants this
AArch64 repair landed first.

## Watchouts

AArch64 still has compatibility fallback paths through `find_value_home` and
legacy `PreparedCallPreservedValue` stack fields. This packet intentionally
keeps the value-home fallback for source endpoints that alias their callee-saved
destination; that fallback is debt until preservation source endpoints can
distinguish the live pre-call value source from the preservation storage slot
for all producers. Stack-slot preservation home population into memory still
flows through existing explicit stack move lowering because
`CallBoundaryMoveInstructionRecord` has no destination-memory field.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_call_boundary_effect_plan|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00040_c|c_testsuite_aarch64_backend_src_00168_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$'`

Result: passed, including all four AArch64 c_testsuite regressions. Proof log:
`test_after.log`.
