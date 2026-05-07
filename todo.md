# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete or Isolate Compatibility Bridge Helpers

## Just Finished

Step 4 repaired the namespaced out-of-class method regression introduced by
fencing current-owner semantic consumers. Owner-scope entry now keeps the final
structured owner `TextId` for qualified owners instead of clearing multi-segment
owners or restoring rendered qualified spelling as current-owner authority.

Added a parser regression for a namespaced out-of-class `operator=` method with
an ambiguous sibling `allocator` name in another namespace. The test verifies
that the parameter type keeps the final owner `TextId` plus namespace context
and that Sema validation succeeds using structured metadata.

Also fenced the remaining direct `typespec_matches_current_struct_local()`
string equality so it only applies when the current owner is backed by a
structured unqualified `TextId`; rendered qualified fallback spelling still
fails closed as semantic current-owner authority.

## Suggested Next

Next packet: continue Step 4 by auditing any remaining parser display fallback
helpers outside the out-of-class owner-scope path, with special attention to
places that still consume compatibility spelling before structured lookup.

## Watchouts

- The repair intentionally preserves only the final owner segment as
  `current_struct_tag_text_id`; it does not reintroduce `ns::Owner` as a
  rendered current-owner fallback.
- `preserve_current_owner_type_metadata()` is limited to unqualified types whose
  `tag_text_id` equals the structured current owner and whose type metadata is
  not already qualified/global.
- `test_after.log` is the canonical proof log for this packet.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 357/357 tests passing.
