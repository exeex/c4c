Status: Active
Source Idea Path: ideas/open/143_stack_layout_id_lookup_helpers_owner.md
Source Plan Path: plan.md
Current Step ID: 2-4
Current Step Title: Move Helper API To Stack Layout Ownership; Update Consumers And Narrow Includes; Prove And Record The Ownership Move

# Current Packet

## Just Finished

Steps 2-4 moved the stack-layout id lookup helpers to stack-layout
ownership and proved the move.

- `find_frame_slot_by_id` and `find_stack_object_by_id` are now declared in
  `src/backend/prealloc/stack_layout/stack_layout.hpp`.
- Their definitions now live in
  `src/backend/prealloc/stack_layout/lookups.cpp`.
- `src/backend/prealloc/prepared_lookups.hpp` and
  `src/backend/prealloc/prepared_lookups.cpp` no longer own those helper
  declarations or definitions.
- `src/backend/prealloc/regalloc/value_homes.cpp` no longer has a private
  duplicate `find_frame_slot_by_id`; it includes the stack-layout owner and
  calls the public helper.
- Direct AArch64 consumers now include the stack-layout owner where they use
  the moved helper declarations. Existing `prepared_lookups.hpp` includes were
  preserved where residual prepared lookup APIs are still used.
- No build-file edits were needed; the existing recursive prealloc source glob
  picked up `stack_layout/lookups.cpp` during the delegated build.

## Suggested Next

Supervisor should review the ownership-move diff and, if accepted, decide the
commit boundary or lifecycle handoff for the active idea.

## Watchouts

- `clang-format` is not installed in this container, so no formatter pass was
  run.
- `memory_store_retargeting.cpp` already directly included
  `stack_layout/stack_layout.hpp`; it did not need a source edit.
- This slice only moved ownership and includes. It did not change stack-layout,
  frame-address, addressing, memory retargeting, comparison, or call-lowering
  behavior.

## Proof

Ran the delegated proof from the repo root:

- `cmake --build --preset default` completed successfully. CMake detected the
  new stack-layout source through the recursive glob and regenerated build
  files automatically.
- `ctest --test-dir build -R '^backend_' --output-on-failure > test_after.log 2>&1`
  completed successfully.

Proof log: `test_after.log`, with `Total Test time (real) = 5.73 sec`.
