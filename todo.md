Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Fold Thin Prepared Lookup Wrappers

# Current Packet

## Just Finished

Step 2 - Fold Thin Prepared Lookup Wrappers completed a narrow mechanical
sub-slice: removed the unused
`prepared_value_id(const module::BlockLoweringContext&, ValueNameId)` declaration
and definition from `dispatch_lookup.hpp`/`.cpp`.

Before removal, `c4c-clang-tool-ccdb function-callers` reported no callers in
the defining translation unit, and targeted `rg` over AArch64 codegen found only
the wrapper declaration/definition. A post-edit exact search for
`prepared_value_id(` found no remaining matches.

## Suggested Next

Continue Step 2 with the remaining mechanical wrappers from the Step 1
inventory, such as the still-outstanding unused
`find_same_block_binary_producer` removal or direct owner-API fold-backs.

## Watchouts

`prepared_named_value_id`, both `find_value_home` overloads,
`find_same_block_named_producer`, `evaluate_same_block_integer_constant`,
`instruction_result_value`, and `instruction_result_value_ref` remain
outstanding. Do not migrate shared authority or touch `prepared_named_value_id`
or `find_value_home` as part of this narrow packet.

## Proof

Ran and passed:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: build succeeded; `ctest -R '^backend_'` passed 169/169 tests. Proof log:
`test_after.log`.
