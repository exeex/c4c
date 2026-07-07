Status: Active
Source Idea Path: ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove The Representative Route

# Current Packet

## Just Finished

Completed Step 4 representative-route proof for `src/20000605-1.c`.

The direct RV64 object route still exits `2` with the same first blocker as
Step 1 and the parent 574 Step 4c evidence:

```text
prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination:
prepared move-bundle classifier rejected ambiguous non-parallel multi-source stack-destination authority
```

Compared with Step 1
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step1/src_20000605-1.c/object-route.log`,
the new direct Step 4 log is byte-identical. Compared with parent 574
`build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/object-route.log`,
the diagnostic content is unchanged; only the wrapper text differs because
Step 4 ran `build/c4cll` directly instead of the CMake torture-case wrapper.

Conclusion: the old
`ambiguous_non_parallel_multi_source_stack_destination` rejection is unchanged;
the representative route did not advance to a later FP cast, runtime mismatch,
pointer, call, select, or unrelated owner, and it did not narrow to a more
specific unsupported stack-destination authority diagnostic.

## Suggested Next

Supervisor should route the next packet back to implementation or plan review:
the representative still fails at the same move-bundle classifier authority, so
Step 4 does not provide closure evidence for this idea.

## Watchouts

- The 574 FP binary acceptance criteria are complete; do not fold this
  move-bundle authority work back into FP binary lowering.
- Do not bypass the prepared move-bundle classifier broadly. The representative
  first blocker is currently a genuine ambiguity in prepared authority, not a
  valid ordered stack-destination move sequence.
- Valid stack-destination multi-source materialization should stay tied to
  select or parallel-copy authority, not filename/function/value-name matching.
- The Step 3 implementation deliberately did not rewrite producer-side
  publication. It narrowed classifier ownership so malformed non-stack
  destination records keep their existing RV64 shape diagnostics, while true
  non-parallel multi-register-source stack-destination bundles remain
  classifier rejections.
- This Step 4 proof shows the representative `src/20000605-1.c` route still
  hits the true non-parallel multi-source stack-destination rejection. Any next
  repair should address that prepared authority shape directly or explicitly
  split/retire the current route; there is no later owner to hand off yet.

## Proof

Delegated representative proof ran and failed with the unchanged expected
blocker; evidence is under
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step4/src_20000605-1.c/`.

```sh
build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000605-1.c -o build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step4/src_20000605-1.c/object-route.o
```

Result: rc `2`; stdout
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step4/src_20000605-1.c/object-route.out`
is empty; stderr/log
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step4/src_20000605-1.c/object-route.err`
and
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step4/src_20000605-1.c/object-route.log`
record the unchanged
`ambiguous_non_parallel_multi_source_stack_destination` diagnostic; rc is saved
at
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step4/src_20000605-1.c/object-route.rc`.

Fresh prepared dump also saved:

```sh
build/c4cll -I /workspaces/c4c --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000605-1.c
```

Result: rc `0`; dump path
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step4/src_20000605-1.c/dump-prepared-bir.txt`.
