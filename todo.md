Status: Active
Source Idea Path: ideas/open/687_structured_layout_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Clarify Aggregate Layout Lookup Boundaries

# Current Packet

## Just Finished

Completed `plan.md` Step 3, `Clarify Aggregate Layout Lookup Boundaries`, with
one behavior-preserving aggregate copy contraction. `append_local_aggregate_copy_to_pointer`
and `append_local_aggregate_copy_to_global` now resolve the source
`AggregateTypeLayout` once at the copy boundary and pass that resolved layout
into leaf collection instead of hiding the rendered-text lookup behind
`collect_sorted_leaf_slots(source_slots)`.

## Suggested Next

Proceed with one more Step 3 packet only if the supervisor wants the remaining
raw-text aggregate lookup sites audited; otherwise route to Step 5 behavior
preservation proof for this runbook.

## Watchouts

This packet intentionally did not change LocalAggregateSlots ownership or carry
layout state into memory helpers. Keep nested `AggregateTypeLayout` ownership
adapter-private, and avoid letting aggregate layout lookup move into public BIR,
target aggregate transport, prepared/prealloc, MIR, byval ABI placement,
initializer, memory/provenance, or call ABI policy.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`;
passed, 100% tests passed, 0 tests failed out of 302. Proof log:
`test_after.log`.
