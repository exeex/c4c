Status: Active
Source Idea Path: ideas/open/687_structured_layout_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Isolate Type Declaration And Typed Operand Compatibility

# Current Packet

## Just Finished

Completed `plan.md` Step 2 selected contraction for recursive local aggregate
slot expansion.

`append_local_aggregate_scalar_slots` now reuses the already-resolved array
element `AggregateTypeLayout` for each element's recursive slot expansion
instead of re-entering the rendered-text overload for every element. The
existing element layout validation, byte offsets, slot naming, and emitted local
slot declarations are intended unchanged.

## Suggested Next

Choose the next structured-layout bridge contraction from the remaining
rendered-text lookup sites, likely struct-field recursion in local aggregate
slot expansion or another `LocalAggregateSlots` path that still only carries
rendered type text.

## Watchouts

This removes only the repeated per-element recursion through the rendered-text
overload after the array element layout has already been resolved. Struct-field
recursion still follows layout field type text, local aggregate copy
source/target size validation still uses the existing rendered-text
compatibility bridge, and unrelated legacy callers still use text-based helpers.
Avoid treating this as a broader policy move into public BIR type/model
authority, prepared/prealloc, target transport, MIR, initializer lowering,
memory/provenance policy, or call ABI placement.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Result: passed; 100% tests passed, 0 tests failed out of 302. Proof log:
`test_after.log`.
