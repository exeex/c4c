Status: Active
Source Idea Path: ideas/open/193_hir_constructor_member_owner_structured_lookup_closure.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Route One Metadata-Rich Owner Lookup Through Structured Identity

# Current Packet

## Just Finished

Step 2 - Route One Metadata-Rich Owner Lookup Through Structured Identity
completed for direct struct constructor lowering.

`try_lower_direct_struct_constructor_call` now treats complete
`tag_text_id`/namespace owner metadata as structured owner metadata. That path
calls `resolve_lowerer_registry_struct_tag(..., "direct-ctor-owner",
&callee_name)`, so complete owner-key resolution uses
`module_->find_struct_def_tag_by_owner` inside the existing helper and fails
closed on a complete structured miss before rendered compatibility can win.

The slice did not change constructor storage, overload ranking, aggregate
lowering, member lowering, or downstream `struct_constructors_` lookup policy.
`callee_name` remains the explicit no-structured-owner compatibility fallback
for the direct constructor route.

## Suggested Next

Run the supervisor review/acceptance path for this Step 2 slice, then choose the
next narrow owner-lookup surface only if the active idea still needs another
metadata-rich route after direct constructors.

## Watchouts

This slice intentionally recognizes only complete tag/namespace owner metadata
for the direct constructor structured path. Incomplete or absent metadata keeps
the existing compatibility behavior. The similar aggregate gate at
`object.cpp:782` was left untouched by packet constraint.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$' > test_after.log 2>&1`

Passed. `test_after.log` is the canonical proof log for this packet.
