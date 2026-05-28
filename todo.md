Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Fold Producer Query Wrappers

# Current Packet

## Just Finished

Step 3 - Fold Producer Query Wrappers completed the assigned
`dispatch_producers.hpp`/`.cpp` sub-slice: removed the public
`find_same_block_binary_producer`, `find_same_block_named_producer`, and
`evaluate_same_block_integer_constant` declarations and definitions.

The remaining AArch64 same-block named producer call sites now call
`mir::find_same_block_named_producer(context.bir_block, ...)` directly. The
comparison hook table uses a private non-capturing adapter because its hook
signature still accepts `BlockLoweringContext`; the adapter forwards only to
the shared MIR query owner.

## Suggested Next

Continue with supervisor-selected Step 3 acceptance review or Step 4
publication result-value helper fold-back.

## Watchouts

Same-block producer behavior remains owned by `mir::query`; no raw BIR scans
or new AArch64 semantic lookup policy were introduced. The comparison hook
field name still mentions `find_same_block_named_producer`, but it no longer
depends on the removed public dispatch producer wrapper.

## Proof

Ran and passed:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: build succeeded; `ctest -R '^backend_'` passed 169/169 tests. Proof log:
`test_after.log`.
