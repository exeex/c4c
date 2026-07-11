Status: Active
Source Idea Path: ideas/open/687_structured_layout_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Isolate Type Declaration And Typed Operand Compatibility

# Current Packet

## Just Finished

Completed `plan.md` Step 2 selected contraction for struct-field recursive
local aggregate slot expansion.

`AggregateField` now retains the already-computed field `AggregateTypeLayout`
when structured and legacy aggregate layouts are built, and
`append_local_aggregate_scalar_slots` reuses that cached field layout for
struct-field recursion instead of re-entering the rendered-text overload. The
existing field layout validation, byte offsets, slot naming, and emitted local
slot declarations are intended unchanged.

## Suggested Next

Choose the next structured-layout bridge contraction from the remaining
rendered-text lookup sites, likely another `LocalAggregateSlots` path that still
only carries rendered type text or a narrowly-owned typed operand compatibility
site from Step 2.

## Watchouts

This removes only struct-field recursion through the rendered-text overload
after the field layout has already been resolved during layout construction.
Array element recursion and target leaf collection already had matching cached
layout contractions from earlier packets. Local aggregate copy source/target
size validation still uses the existing rendered-text compatibility bridge, and
unrelated legacy callers still use text-based helpers. Avoid treating this as a
broader policy move into public BIR type/model authority, prepared/prealloc,
target transport, MIR, initializer lowering, memory/provenance policy, or call
ABI placement.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Result: passed; 100% tests passed, 0 tests failed out of 302. Proof log:
`test_after.log`.
