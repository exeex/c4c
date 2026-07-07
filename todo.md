Status: Active
Source Idea Path: ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove The Representative Route

# Current Packet

## Just Finished

Step 5 reran the representative RV64 object route for `src/20000605-1.c`
under
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step5/src_20000605-1.c/`.
The route still exits 2 on the same prepared move bundle, but the old generic
ambiguity is now narrowed at the RV64 prepared consumer: the shared
`prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
category remains, while the diagnostic text and
`fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`
identify the missing producer authority for register fan-in to one stack
destination.  The route has not advanced to a later FP cast, runtime mismatch,
pointer, call, select, or unrelated owner.

## Suggested Next

Proceed to Step 6: run the backend guard and closure-readiness proof selected
by the supervisor, including at least the focused RV64 object-emission test and
the broader backend subset required before lifecycle closure.

## Watchouts

- Keep this mapped to
  `ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md`.
- Do not bypass the prepared move-bundle classifier broadly or key behavior to
  `src/20000605-1.c`, `render_image_rgb_a`, temporary names, or wrapper text.
- The Step 5 prepared dump still shows `phase=before_instruction
  authority=none block_index=1 instruction_index=2` with moves from
  `value_id=22` and `value_id=23` to stack-slot `value_id=24`; this proof
  confirms narrowing, not semantic lowering or later-route advancement.
- The earliest shared classifier enum/category remains
  `ambiguous_non_parallel_multi_source_stack_destination`; the narrower owner
  is currently exposed through RV64 prepared-consumer diagnostic text and
  `fragment_status`.

## Proof

Object route proof:
`build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000605-1.c -o build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step5/src_20000605-1.c/object-route.o`
failed with exit code 2 as recorded in
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step5/src_20000605-1.c/object-route.rc`;
stdout, stderr, and combined logs are saved as `object-route.out`,
`object-route.err`, and `object-route.log` in the same directory.

Supporting prepared dump:
`build/c4cll -I /workspaces/c4c --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000605-1.c`
passed with exit code 0 as recorded in
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step5/src_20000605-1.c/dump-prepared-bir.rc`;
the dump is saved as `dump-prepared-bir.txt` with stderr in
`dump-prepared-bir.err`.
