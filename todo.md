# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow In X86 packet by extending
generic compare-branch target ownership into one more x86 consumer seam:
`src/backend/prealloc/prealloc.hpp` now exposes a shared lookup that resolves
prepared compare-branch target labels directly from a function control-flow
record plus source block, `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
now prefers that prepared target-label contract for the plain compare-driven
local-guard branch plan instead of routing from mutated BIR terminator labels
when prepared metadata exists, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` now proves the immediate
local-slot guard route still emits the canonical asm after carrier-label
rewrites are introduced only in the prepared BIR topology.

## Suggested Next

Stay in Step 3 and tighten the remaining compare-driven entry seam by moving
more plain local-guard branch consumers off emitter-local compare/branch
reconstruction and onto one prepared branch-plan helper, especially where x86
still derives compare setup or fallback routing directly from block-local
terminator state.

## Watchouts

- Keep this route in Step 3 consumer work; do not widen into Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- This packet only moves plain local-guard branch target ownership onto the
  prepared contract; compare opcode rendering for broader compare families is
  still local and should be generalized semantically rather than with new named
  lanes.
- The new shared helper is target-label focused; do not use it to justify
  emitter-local CFG recovery or testcase-shaped branch-plan shortcuts in
  adjacent routes.
- The broader `^backend_` checkpoint still has the same four known failures in
  variadic and dynamic-member-array semantic lowering outside this packet's
  owned files.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3 handoff proof passed and preserved `test_after.log` at the
repo root.
