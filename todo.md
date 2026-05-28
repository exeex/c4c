Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prune Public Surfaces And Prove The Slice

# Current Packet

## Just Finished

Step 5 - Prune Public Surfaces And Prove The Slice completed the assigned
public-surface cleanup for the dispatch lookup, producer, and publication
fold-back slice.

Removed the stale public `prepared_named_value_id` declaration/definition from
`dispatch_lookup.hpp`/`.cpp`, pruned AArch64 call-site includes that only used
that exported wrapper, and kept the remaining prepared-name guard as private
translation-unit convenience local to the immediate lowering consumers. Header
inspection shows no folded helper declarations for `prepared_named_value_id`,
`prepared_value_id`, `find_value_home`, `find_same_block_binary_producer`,
`evaluate_same_block_integer_constant`, `instruction_result_value`, or
`instruction_result_value_ref` remain in the AArch64 dispatch-family public
headers.

## Suggested Next

Ask the plan owner to decide whether idea 60 can close, deactivate, or needs a
follow-up lifecycle split after supervisor review of the completed runbook.

## Watchouts

Remaining `dispatch_lookup.hpp` exports are not the folded lookup wrappers:
`make_named_prepared_result_register`, `emitted_scalar_value_available`,
`is_scalar_call_argument_producer_opcode`, `find_same_block_scalar_producer`,
and `has_same_block_load_local_producer` still carry AArch64 register/home or
scalar-producer policy and should be handled only by later source ideas if they
are to move.

`comparison.hpp` still has a `find_same_block_named_producer` hook field in
`PreparedComparisonHooks`; that is dependency injection for comparison lowering,
not a dispatch producer wrapper declaration.

## Proof

Ran and passed:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: build succeeded; `ctest -R '^backend_'` passed 169/169 tests. Proof log:
`test_after.log`.
