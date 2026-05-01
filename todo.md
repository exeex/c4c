# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Completed Step 3.3 method-owner cleanup by keeping the Sema
`resolve_owner_by_rendered_name_fallback(owner, namespace_context_id)` helper
deleted and adding parser-owned structured owner metadata to out-of-class
`NK_FUNCTION` nodes.

Ordinary qualified declarators now thread their parsed `QualifiedNameRef` out of
`parse_declarator()`, and top-level function construction attaches the owner
qualifier `TextId` path plus the resolved owner namespace context to the
function node. Special out-of-class operator and constructor paths attach the
same carrier from their already-consumed owner tokens. Sema owner resolution now
uses `method_owner_key_from_qualifier()`, `struct_defs_by_key_`, and the direct
method-owner record path without rendered owner recovery.

Focused coverage was strengthened for stale rendered owner disagreement: the
synthetic Sema method-owner test now includes a real `StaleOwner` record, and
the parser operator/constructor tests assert that out-of-class function nodes
carry structured owner and base `TextId` metadata.

## Suggested Next

Continue Step 3.3 with the remaining static-member rendered owner bridge in
`lookup_struct_static_member_type(tag, reference)` and
`static_member_lookup_has_structured_metadata()`, using focused stale rendered
owner/member coverage before removing or narrowing that route.

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
passing delegated positive Sema tests, including the previously failing
`cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp` and the
same-feature method-owner coverage in
`cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp`,
`cpp_positive_sema_inherited_implicit_member_out_of_class_runtime_cpp`, and
`cpp_positive_sema_operator_implicit_member_runtime_cpp`.
