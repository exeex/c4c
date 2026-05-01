# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Completed Step 3.3 instance-field cleanup by deleting the Sema
`has_struct_instance_field_legacy(tag, member)` rendered owner/member route and
the rendered `struct_field_names_` / `struct_base_tags_` maps. Implicit
out-of-class method field validation now resolves through
`current_method_struct_key_`, structured base keys, and field `TextId` metadata
when present.

Kept behavior for parsed positive cases whose field declarations or references
do not carry `TextId` yet by adding a structured-owner-key field-name
compatibility map. That compatibility is keyed by `SemaStructuredNameKey`, so it
does not recover fields from stale rendered owner tags or cross-namespace owner
spellings.

## Suggested Next

Continue Step 3.3 with the remaining method-owner route
`resolve_owner_by_rendered_name_fallback(owner, namespace_context_id)`, after
confirming each call site has a reliable qualifier `TextId`/namespace-context
owner key or a documented no-metadata compatibility case.

## Watchouts

- `lookup_struct_static_member_type(tag, reference)` still accepts rendered
  owner text only to bridge qualifier spellings to the existing structured
  record key when the reference node does not directly provide the complete
  namespace-aware owner key. Do not count renaming that bridge as progress.
- `static_member_lookup_has_structured_metadata()` still uses the owner text for
  the unresolved-template optimistic path. Treat any further shrink there as a
  separate packet with focused coverage.
- The instance-field `TextId` carrier is incomplete for some parsed positive
  cases. The new name compatibility path is intentionally owner-keyed; do not
  convert it back into tag/member lookup while working on method-owner cleanup.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 884/884
passing delegated positive Sema tests, including the same-feature implicit
method-field coverage in
`cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp`,
`cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp`,
`cpp_positive_sema_inherited_implicit_member_out_of_class_runtime_cpp`, and
`cpp_positive_sema_operator_implicit_member_runtime_cpp`.
