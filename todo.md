Status: Active
Source Idea Path: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Structured Mirrors for Method Lookup

# Current Packet

## Just Finished

Started Step 4 by adding write-only structured owner-key mirrors for struct method registration while leaving existing rendered string maps and all method lookup readers authoritative.

Added `HirStructMethodLookupKey`, keyed by `HirRecordOwnerKey`, method `TextId`, and constness, plus lowerer-private mirrors for `struct_methods_`, `struct_method_link_name_ids_`, and `struct_method_ret_types_`. Ordinary struct method registration and instantiated template struct method registration now dual-write these mirrors when owner and method metadata are complete.

## Suggested Next

Continue Step 4 by adding structured dual-read/parity helpers for method lookup that compare structured mirrors against the existing rendered string maps without changing the return behavior of `find_struct_method_mangled`, `find_struct_method_link_name_id`, or `find_struct_method_return_type`.

## Watchouts

This slice does not route any reader through the structured mirrors and does not touch member or static-member lookup maps. Ref-overloaded methods still share the existing rendered key authority; the new mirror tracks the same owner/method/constness identity currently represented by `struct_tag::method[_const]`, not a fuller overload signature.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports 73/73 HIR-labeled tests passing.
