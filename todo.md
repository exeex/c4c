# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Stand Up Frame And Memory Lowering Owners
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed step 1.5.3 by confirming that the remaining broad-header consumers
in `module/module_emit.cpp`, `route_debug.cpp`, and the three backend handoff
boundary tests are honest prepared-route compatibility holdouts rather than
missed reviewed-owner narrowing opportunities, then split step 2 into
execution-sized lowering-family substeps so the next packet can start real
canonical lowering migration.

## Suggested Next

Start step 2.1 by standing up the reviewed `lowering/frame_lowering.*` and
`lowering/memory_lowering.*` seams, moving one coherent frame-home or
memory-operand helper family behind those owners while keeping legacy and
prepared callers compiling through explicit forwarding.

## Watchouts

- Keep `module_emit.cpp` and `route_debug.cpp` classified as prepared-route
  compatibility holdouts until real reviewed `prepared/` owners exist; do not
  fake step-2 progress by inventing new prepared headers.
- Treat step 2 as canonical lowering migration, not public-entry or prepared
  adapter rewiring; keep `emit.cpp`, `prepared_module_emit.cpp`, and
  route-debug surfaces wrapper-thin.
- Start with frame and memory services because later call, comparison, and
  prepared packets depend on canonical frame-home and operand helpers.
- Preserve the legacy `x86::emit_module(...)`, `x86::assemble_module(...)`,
  and `x86::emit_prepared_module(...)` compatibility surfaces until step 4
  explicitly rewires live entry ownership.
- Reject testcase-shaped matcher growth while moving helpers: the packet should
  relocate semantic lowering owners, not add new named-case shortcuts.

## Proof

Step 1.5.3 classification closeout and step-2 runbook split on 2026-04-22.
Latest code proof remains the last accepted backend subset run:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed. Canonical log paths: `test_before.log`, `test_after.log`
