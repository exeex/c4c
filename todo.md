Status: Active
Source Idea Path: ideas/open/687_structured_layout_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Isolate Type Declaration And Typed Operand Compatibility

# Current Packet

## Just Finished

Completed `plan.md` Step 2 selected contraction for byval aggregate parameter
alias materialization.

`materialize_aggregate_param_aliases` now passes the already-computed
`AggregateParamInfo::layout` into local aggregate slot declaration and leaf
collection instead of re-entering the root rendered-text lookup through
`declare_local_aggregate_slots(info.type_text, ...)`. Existing text-based
aggregate declaration and slot expansion helpers remain available for legacy
callers, and the produced local slots plus load/store copy behavior are
intended unchanged.

## Suggested Next

Choose the next structured-layout bridge contraction from the remaining
rendered-text lookup sites, likely local aggregate copy validation or another
`LocalAggregateSlots` path that still only carries rendered type text.

## Watchouts

The new layout-taking helpers only remove the root lookup for byval parameter
alias setup. Recursive child expansion still follows existing layout field
type text, and unrelated legacy callers still use the text-based helpers.
Avoid treating this as a broader policy move into public BIR type/model
authority, prepared/prealloc, target transport, MIR, initializer lowering,
memory/provenance policy, or call ABI placement.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Result: passed; all 302 selected backend tests passed. Proof log:
`test_after.log`.
