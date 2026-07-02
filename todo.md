Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize Or Reclassify The Single-Move Shape

# Current Packet

## Just Finished

Step 3 tightened and proved the RV64 object-emission boundary for the prepared
register-source to stack-destination move shape.

Implemented boundary:

- `fragment_for_prepared_move_bundle(...)` now materializes
  `reason=consumer_register_to_stack` only when the move has no destination
  register metadata, no explicit destination stack offset, no immediate or
  parallel-copy source metadata, a real RV64 GPR register source home, coherent
  optional storage-plan register evidence, an explicit stack-slot destination
  home, and matching source/destination scalar byte sizes.
- Source size authority comes from the source BIR type when available, or from
  explicit `PreparedValueHome::size_bytes` for focused prepared fixtures that
  have no BIR source definition. Missing size authority remains rejected.
- Existing same-scalar `consumer_stack_to_stack` materialization still delegates
  to the stack-slot helper and was not widened to accept register sources.
- Existing mixed bundles with one accepted `consumer_register_to_stack` move
  plus coherent stack-to-stack moves remain accepted, but bundles with multiple
  `consumer_register_to_stack` moves remain rejected.
- RV64 diagnostics now print source/destination home kinds and BIR types when
  available so producer-reason and extension-authority gaps are visible.

Representative results:

- `tests/c/external/gcc_torture/src/pr27073.c` remains rejected with
  `reason=consumer_stack_to_stack`, `source_home_kind=register`,
  `source_type=i16`, `destination_home_kind=stack_slot`, and
  `destination_type=i32`; this is a producer-owned reason plus conversion
  authority gap, not an RV64 stack-to-stack materialization opportunity.
- `tests/c/external/gcc_torture/src/20010518-1.c` remains rejected with
  `move_count=2` and two register-source `consumer_stack_to_stack` moves
  targeting the same stack destination; this remains the multi-move ownership
  problem for Step 5 or a follow-up producer contract.
- Focused RV64 object-emission coverage now publishes explicit source-size
  authority for the accepted GPR-to-stack fixture.

## Suggested Next

Proceed to Step 4 by adding or reviewing focused reject coverage for adjacent
malformed register-source stack-destination shapes, especially multiple
`consumer_register_to_stack` moves and missing source-size authority.

## Watchouts

Do not convert the `pr27073.c` diagnostic into an accepted store without an
explicit producer-owned converted source/result or corrected reason authority.
The mixed accepted bundle is legacy stack-to-stack preservation plus one
register-to-stack move; it is not permission to accept arbitrary multi-register
source bundles.

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_'`; both completed
successfully with final output captured in `test_after.log`.

Also ran focused object-route probes:

- `./build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr27073.c -o /tmp/c4c_pr27073.o`
  rejected with the producer reason/type-authority diagnostic above.
- `./build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20010518-1.c -o /tmp/c4c_20010518-1.o`
  rejected with the multi-move register-source diagnostic above.
