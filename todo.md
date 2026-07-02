Status: Active
Source Idea Path: ideas/open/515_rv64_bankless_conversion_adjacent_stack_slot_moves.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Bankless And Conversion Boundaries

# Current Packet

## Just Finished

Step 2 - Define The Bankless And Conversion Boundaries is complete for
idea 515.

Accepted same-scalar stack-slot boundary:

- RV64 may materialize `consumer_stack_to_stack` only when the existing
  `fragment_for_prepared_stack_slot_to_stack_slot_move` predicate remains
  true: no parallel-copy bundle, before-instruction `authority=none`, a single
  plain `Move`, stack-slot source and destination homes, coherent storage-plan
  endpoints, `bank=gpr`, `encoding=frame_slot`, `width=1`, slot and stack
  offset present for both endpoints, matching scalar BIR types, supported RV64
  scalar memory size, and a scratch GPR.
- This accepted predicate is intentionally the idea 513 same-scalar copy
  contract. It must not be widened to infer GPR bank from scalar type inside
  RV64 object emission, and it must not treat different source/destination
  scalar types as a byte copy.

Bankless frame-slot boundary:

- `bank=none` on a prepared `encoding=frame_slot` scalar value whose BIR type
  and stack object are known is a prepared storage-plan publication omission,
  not an RV64 materialization authority. The repair owner is
  `src/backend/prealloc/storage_plans.cpp`, where `build_storage_plan_value`
  currently seeds bank from a typed regalloc value and can fall back to
  `void` before publishing a frame-slot value such as `%t8`.
- RV64 object emission remains fail-closed for missing or incoherent storage
  authority. If the producer still publishes `bank=none`, missing
  `slot_id`/`stack_offset`, non-frame-slot encoding, unsupported bank, or
  unsupported width, the diagnostic owner should be RV64 prepared
  move-bundle rejection before generic stack-copy materialization, not a raw
  stack copy.

Conversion-adjacent stack-slot boundary:

- The `%t8 i16 -> %t9 i64` movement has no explicit conversion authority in
  the prepared move bundle. The current facts are
  `move_bundle phase=before_instruction authority=none block_index=0
  instruction_index=14`, `op_kind=move`, `reason=consumer_stack_to_stack`,
  source home `stack_slot` type `i16`, destination home `stack_slot` type
  `i64`, `parallel_copy=no`, `select_edge_suppression_authorized=no`, and
  `cast_dependency_stack_publication_authorized=no`.
- Because the semantic instruction at index 14 is `bir.zext i16 %t8 to i64`,
  a stack-slot-to-stack-slot move for `%t8 -> %t9` is conversion-adjacent.
  The existing move record only authorizes value movement; it does not
  authorize zero-extension, sign-extension, truncation, or any other
  conversion semantics.
- The accepted Step 2 decision is therefore to reject this family before the
  generic stack-copy materializer unless a future producer publishes an
  explicit conversion materialization contract. The owner-specific diagnostic
  should live in the RV64 prepared move-bundle fragment rejection path near
  `rv64_prepared_move_bundle_fragment_failure_diagnostic`, analogous to the
  existing register-source stack-destination conversion classifier diagnostic,
  but for stack-slot source plus stack-slot destination mismatched scalar
  types.

Adjacent shapes that must remain fail-closed:

- `bank=none` frame slots when the producer cannot prove scalar bank
  authority.
- Any conversion-adjacent stack-slot movement with mismatched scalar types and
  only `op_kind=move`/`authority=none`.
- Contradictory home and storage-plan facts, missing storage-plan endpoint
  facts, unsupported banks such as `fpr`, `vreg`, or `aggregate_address`,
  unsupported `contiguous_width`, missing slot/offset facts, pointer or
  aggregate-like values outside `rv64_scalar_memory_size_for_type`, multi-move
  non-parallel stack-destination bundles, and register-source cases already
  owned by idea 514.

## Suggested Next

Proceed to Step 3 by repairing the prepared storage-plan producer so known
scalar frame-slot values such as `%t8` publish a coherent GPR bank from real
BIR/home authority. Keep RV64 object emission fail-closed for any remaining
`bank=none` or incoherent frame-slot storage facts.

## Watchouts

Repairing `%t8` from `bank=none` to `bank=gpr` will not make `pr69447.c`
accepted: the remaining `%t8 i16 -> %t9 i64` stack-slot move still lacks
conversion authority and should move to an owner-specific RV64 diagnostic
instead of `generic_move_bundle_materialization_failed`. Do not encode
`zext` semantics as a same-scalar stack copy.

## Proof

Evidence-only packet. Probes used:

- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu --mir-focus-function foo --mir-focus-value t8 tests/c/external/gcc_torture/src/pr69447.c`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu --mir-focus-function foo --mir-focus-value t9 tests/c/external/gcc_torture/src/pr69447.c`
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/pr69447.c -o /tmp/pr69447_step2_probe.o`
- `c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp fragment_for_prepared_stack_slot_to_stack_slot_move build/compile_commands.json`
- `c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp rv64_prepared_move_bundle_fragment_failure_diagnostic build/compile_commands.json`

No code-changing proof was required, and `test_after.log` was intentionally
untouched per the delegation. `git diff --check -- todo.md` passes.
