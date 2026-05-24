Status: Active
Source Idea Path: ideas/open/aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Absorb One Bounded Helper Family

# Current Packet

## Just Finished

Step 2 absorbed the before-call move sequencing helper family from the dispatch
bridge into the call-move owner.

Completed work:

- Moved `move_source_aliases_destination`,
  `call_boundary_move_reloads_materialized_address`, and
  `order_before_call_moves_for_source_preservation` from
  `calls_dispatch_bridge.cpp` to `calls_moves.cpp`.
- Kept `move_source_aliases_destination` private to `calls_moves.cpp`.
- Removed the two dispatch-facing helper declarations from
  `calls_dispatch_bridge.hpp`.
- Declared `call_boundary_move_reloads_materialized_address` and
  `order_before_call_moves_for_source_preservation` in `calls.hpp`, because
  `dispatch.cpp` still calls them while call-boundary sequencing remains split
  across dispatch and call moves.
- Added the needed `dispatch_publication_common.hpp` include to
  `calls_moves.cpp` for `registers_alias`.

Owner rationale:

`calls_moves.cpp` already owns `lower_before_call_moves`,
`lower_after_call_moves`, and `CallBoundaryMoveInstructionRecord`
construction. The relocated helpers only inspect before-call move records and
materialized-address records to preserve source ordering and skip redundant
reloads, so the call-move owner is the durable home. No recursive scalar
argument, indirect callee, missing frame-slot argument, call-result recording,
or stack-preservation helpers were absorbed.

## Suggested Next

Supervisor can either review and commit this completed Step 2 slice or delegate
Step 3 only if a nearby same-shape bridge helper is selected with an equally
bounded owner/proof surface.

## Watchouts

- Keep the work behavior-preserving.
- Do not broaden into `dispatch.cpp`, whole-call-family cleanup, phase
  extraction, ABI redesign, or expectation rewrites.
- The remaining bridge helpers have wider semantic or publication surfaces than
  the pure before-call move sequencing family; do not absorb them without a
  separately delegated packet and proof.

## Proof

Delegated proof passed and was written to `test_after.log`:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_prepare_frame_stack_call_contract|backend_call_boundary_effect_plan|backend_aarch64_instruction_dispatch)$"; } 2>&1 | tee test_after.log'`

Focused subset result: 5/5 tests passed.

`git diff --check` passed.
