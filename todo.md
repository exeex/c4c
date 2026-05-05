# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's template owner/member-context aggregate identity fallout is repaired
for the parser/Sema-owned member-expression family without reintroducing
`TypeSpec::tag`. Record body parsing now installs a scoped structured
`record_def` carrier for the current record and its template-origin spelling,
so constructor params, member params, locals, and implicit member bases keep
aggregate identity through parser/Sema/HIR.

Nested records declared inside an active record body now upgrade the same-scope
tag-only binding to a concrete `record_def` binding after finalization. The
upgrade is scoped and guarded so local current-record template-id heads such as
`reverse_iterator<Iter>` still remain on the existing template-id parser path
instead of being consumed as plain record names.

This fixed the dominant member-expression base failures, including
`template_inline_method_member_context_frontend_cpp`, copy/move constructor and
assignment cases, iterator member-operator cases, range-for member cases, and
related by-value struct operator cases.

## Suggested Next

Next packet should target the remaining LIR signature mirror boundary for
template-instantiated aggregate parameters. HIR now prints structured aggregate
parameters such as `sum_i(p: struct Pair_T_int)`, but
`LirFunction.signature_param_type_refs` is still rendered from a type carrier
whose aggregate spelling collapses to `i32`.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Keep the record-body carrier scoped. The local `record_def` shortcut in
  `parse_base_type` is intentionally limited to same-scope visible typedef
  bindings and excludes heads followed by `<`.
- The remaining LIR failures are not a parser member-base carrier issue:
  `cpp_positive_sema_template_fn_struct_cpp` and
  `cpp_positive_sema_template_struct_nested_cpp` still fail at
  `LirFunction.signature_param_type_refs` mirror construction.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: improved baseline, with no new failures.

`cmake --build build` passed. The delegated CTest command exits non-zero
because this positive/Sema subset is still red, but it improved
`test_before.log` from 838/884 passing with 46 failures to 876/884 passing with
8 failures. Failure comparison found 38 fixed tests and 0 new failures.

Canonical proof log: `test_after.log`.
