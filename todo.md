# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Completed Step 3.3 static member cleanup by deleting the Sema
`lookup_struct_static_member_type_legacy(tag, member)` rendered mirror route and
the rendered `struct_static_member_types_` map. Qualified static member lookup
now resolves through the existing structured record key plus member `TextId`
metadata and returns no static-member type from the removed rendered mirror.

Added `structured_static_member_lookup_runtime.cpp` to keep namespace-qualified
same-feature static member lookup covered after the mirror deletion.

## Suggested Next

Continue Step 3.3 with one focused non-static Sema route: either the instance
field `has_struct_instance_field_legacy(tag, member)` rendered probe or
`resolve_owner_by_rendered_name_fallback(owner, namespace_context_id)`, after
confirming the chosen call site already has a reliable owner key or direct
record metadata.

## Watchouts

- `lookup_struct_static_member_type(tag, reference)` still accepts rendered
  owner text only to bridge qualifier spellings to the existing structured
  record key when the reference node does not directly provide the complete
  namespace-aware owner key. Do not count renaming that bridge as progress.
- `static_member_lookup_has_structured_metadata()` still uses the owner text for
  the unresolved-template optimistic path. Treat any further shrink there as a
  separate packet with focused coverage.
- Instance-field and method-owner routes still have rendered fallback helpers;
  delete only the next route whose producer metadata is already present.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 884/884
passing delegated positive Sema tests, including
`cpp_positive_sema_structured_static_member_lookup_runtime_cpp`.
