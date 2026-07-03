Status: Active
Source Idea Path: ideas/open/569_prepared_move_bundle_ambiguous_multi_source_stack_destination.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Pin The Prepared Classifier Boundary

# Current Packet

## Just Finished

Step 1 pinned the prepared classifier boundary for
`tests/c/external/gcc_torture/src/20001026-1.c`; evidence is under
`build/agent_state/569_step1_prepared_classifier_boundary/`.

Fresh focused results:

- `cmake --build build --target c4cll`: pass, `ninja: no work to do`.
- `dump-bir`, `dump-prepared-bir`, and `dump-mir` commands for
  `--target riscv64-linux-gnu`: exit 0; outputs are
  `dump-bir.txt`, `dump-prepared-bir.txt`, and `dump-mir.txt`.
- RV64 gcc_torture object runner: exit 1 in `object-route.log` with
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.

Concrete prepared shape:

- Function: `real_value_from_int_cst`.
- Producer/block context: prepared `block_index=3`, mapped by
  `--- prepared-control-flow ---` to `block_1`.
- The failing family is the select-materialization publications in `block_1`:
  prepared instruction indexes 7, 10, and 13 compute `%t12.store0`,
  `%t12.store1`, and `%t12.store2`; the paired store publications at indexes
  8, 11, and 14 record `source_producer=select_materialization`.
- Each relevant move bundle is `phase=before_instruction authority=none`,
  `destination_kind=value destination_storage=stack_slot`, and has multiple
  moves into one stack-destination value:
  - `block_index=3 instruction_index=7`: destination `value_id=17`
    `%t12.store0`, slot #25 offset 104; sources `value_id=11` `%t11`
    register `t0`, `value_id=15` `%t13` register `s2`, and `value_id=16`
    `%t12.elt0` stack slot #24 offset 96; reasons are two
    `consumer_register_to_stack` moves plus one `consumer_stack_to_stack` move.
  - `block_index=3 instruction_index=10`: destination `value_id=19`
    `%t12.store1`, slot #27 offset 120; sources `value_id=11` `%t11`
    register `t0`, `value_id=15` `%t13` register `s2`, and `value_id=18`
    `%t12.elt1` stack slot #26 offset 112; same reason mix.
  - `block_index=3 instruction_index=13`: destination `value_id=21`
    `%t12.store2`, slot #29 offset 136; sources `value_id=11` `%t11`
    register `t0`, `value_id=15` `%t13` register `s2`, and `value_id=20`
    `%t12.elt2` stack slot #28 offset 128; same reason mix.

Rejecting path:

- Classifier: `classify_prepared_object_move_bundle_consumer()` in
  `src/backend/prealloc/prepared_object_traversal.cpp`.
- Predicate: `prepared_move_bundle_has_ambiguous_multi_source_stack_destination()`.
- Diagnostic mapper:
  `diagnose_prepared_object_consumer(const PreparedObjectMoveBundleConsumerClassification&)`.
- Reason fields involved: non-parallel `authority_kind=None`,
  `BeforeInstructionCopies`/`BeforeInstruction` phase, move count at least 2,
  register source homes, stack-slot destination homes, and either same
  `to_value_id` or same destination stack home. The 20001026-1.c bundles trip
  this on the two register-source moves into the same stack-destination value.

## Suggested Next

Execute Step 2 in `plan.md`: add focused prepared-layer contract coverage for
the general non-parallel multi-source stack-destination select-materialization
shape. The fixture should describe the move-bundle shape and source/destination
homes, not `src/20001026-1.c` by name.

## Watchouts

- Do not treat this as integer div/rem lowering until the prepared classifier boundary is crossed.
- The current first owner is still prepared move-bundle classification; no
  representative reached runtime mismatch, timeout, segfault, or test
  infrastructure failure.
- The `--dump-mir` route only produced the existing x86/debug summary surface,
  so object-route evidence remains the meaningful RV64 failure proof.
- `c4c-clang-tool-ccdb list-symbols` could not load a compile command for
  `src/backend/prealloc/prepared_object_traversal.cpp` from
  `build/compile_commands.json`; helper names are source/rg-backed and the
  failed clang-tool outputs are recorded in the evidence directory.
- Do not change runtime comparison, expected output, unsupported markers, or allowlists.
- Do not special-case `src/20001026-1.c` by name.
- Keep generic `unsupported_instruction_fragment` rows in idea 570, not this active plan.

## Proof

Step 1 proof:

- `cmake --build build --target c4cll`
- Focused commands recorded in
  `build/agent_state/569_step1_prepared_classifier_boundary/commands.sh.txt`.
- Boundary summary:
  `build/agent_state/569_step1_prepared_classifier_boundary/boundary_summary.md`.
- Return-code files:
  `dump-bir.rc`, `dump-prepared-bir.rc`, `dump-mir.rc`, `object-route.rc`.
- `git diff --check -- todo.md`
- `scripts/plan_review_state.py show`
