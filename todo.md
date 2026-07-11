Status: Active
Source Idea Path: ideas/open/687_structured_layout_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Isolate Type Declaration And Typed Operand Compatibility

# Current Packet

## Just Finished

Completed `plan.md` Step 2 selected contraction for local aggregate copy leaf
collection.

`append_local_aggregate_copy_from_slots` now reuses the already-resolved target
`AggregateTypeLayout` when collecting target leaves instead of calling the
rendered-text `collect_sorted_leaf_slots(target_slots)` entrypoint. The
existing source/target compatibility decision, slot type checks, and emitted
load/store copy sequence are intended unchanged.

## Suggested Next

Choose the next structured-layout bridge contraction from the remaining
rendered-text lookup sites, likely recursive local aggregate slot expansion or
another `LocalAggregateSlots` path that still only carries rendered type text.

## Watchouts

This removes only the redundant target leaf-collection lookup after local copy
compatibility has already resolved the target layout. Source/target size
validation still uses the existing rendered-text compatibility bridge, recursive
child expansion still follows layout field type text, and unrelated legacy
callers still use text-based helpers. Avoid treating this as a broader policy
move into public BIR type/model authority, prepared/prealloc, target transport,
MIR, initializer lowering, memory/provenance policy, or call ABI placement.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Result: passed; 100% tests passed, 0 tests failed out of 302. Proof log:
`test_after.log`.
