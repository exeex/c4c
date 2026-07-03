Status: Active
Source Idea Path: ideas/open/569_prepared_move_bundle_ambiguous_multi_source_stack_destination.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Prepared Move-Bundle Classification

# Current Packet

## Just Finished

Step 3 repaired prepared move-bundle classification for the pinned
multi-source stack-destination select-materialization shape.

Implementation rule:

- `BeforeInstructionCopies` traversal events now retain their BIR instruction
  pointer, so the classifier can distinguish actual select-materialization
  sites from unrelated before-instruction copy bundles.
- The existing ambiguous multi-source stack-destination predicate still
  catches ordinary non-parallel bundles with multiple register-source moves
  into one stack destination.
- A narrow select-materialization shape is accepted instead of rejected:
  actual `bir::SelectInst` event, `authority_kind=None`,
  `PreparedMovePhase::BeforeInstruction`, stack-slot destination moves, at
  least two register-source homes, at least one stack-source home, and all
  moves targeting the same destination value or same destination stack home.
- No runtime expectations, allowlists, unsupported markers, generic
  `unsupported_instruction_fragment` diagnostics, or idea/plan files were
  changed.

Focused test disposition:

- `verify_move_bundle_consumer_accepts_select_materialization_stack_destination_sources()`
  now asserts the intended contract: the three-source select-materialization
  stack-destination shape is `Available`, preserves `move_count == 3`, exposes
  the select instruction on the traversal event, and produces no diagnostic.
- `verify_move_bundle_consumer_rejects_ambiguous_multi_source_stack_destination()`
  remains in place to prove the simpler two-register-source ambiguous bundle
  still fails closed.

Evidence:

- `build/agent_state/569_step3_prepared_classifier_repair/repair_summary.md`
- `build/agent_state/569_step3_prepared_classifier_repair/commands.sh.txt`
- `build/agent_state/569_step3_prepared_classifier_repair/clang_symbols.out`
- `build/agent_state/569_step3_prepared_classifier_repair/backend_full.test_after.log`

## Suggested Next

Execute Step 4 in `plan.md`: rerun `src/20001026-1.c` through the RV64
gcc_torture object route, confirm the old
`AmbiguousNonParallelMultiSourceStackDestination` diagnostic is gone or record
the exact remaining blocker, and classify any next owner separately.

## Watchouts

- Do not treat this as integer div/rem lowering until the prepared classifier boundary is crossed.
- This packet only repairs the prepared classifier boundary; Step 4 owns the
  representative RV64 object-route disposition and any later owner
  classification.
- The new acceptance is gated on an actual select instruction plus a stack
  source in the same stack-destination bundle; unrelated two-register-source
  ambiguous bundles remain rejected.
- Do not change runtime comparison, expected output, unsupported markers, or allowlists.
- Do not special-case `src/20001026-1.c` by name.
- Keep generic `unsupported_instruction_fragment` rows in idea 570, not this active plan.

## Proof

Step 3 proof:

- `cmake --build build --target c4cll`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- `git diff --check -- todo.md tests src`
