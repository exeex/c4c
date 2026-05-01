# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Continued Step 3.3 static-member cleanup by deleting the
`static_member_owner_key(tag, reference)` helper that blended reference-carried
owner metadata with rendered owner text. `lookup_struct_static_member_type()`
now tries the reference owner/member key first and treats non-template
structured misses as authoritative; rendered-tag lookup is narrowed to a
successful compatibility hit when the reference cannot provide the complete
static-member owner key.

`static_member_lookup_has_structured_metadata()` now blocks the optimistic
rendered-owner path when a structured reference key has table authority or when
the rendered tag resolves to a different structured owner key. Template
instantiation references keep the existing no-metadata optimistic route for
unresolved-template static members. Added focused stale rendered-owner/member
Sema coverage for a reference whose structured owner/member metadata disagrees
with the rendered `Owner::member` spelling.

## Suggested Next

Continue Step 3.3 by reviewing the remaining Sema rendered-spelling
compatibility routes outside static-member lookup, with special attention to
template-instantiated references where structured owner keys are still
incomplete and the no-metadata optimistic route remains intentional.

## Watchouts

- `lookup_struct_static_member_type(tag, reference)` still has a rendered-tag
  compatibility hit path when `static_member_owner_key_from_reference()` cannot
  build a valid owner key. This is materially narrower than the deleted blended
  helper, but not a complete rendered-tag removal.
- Template-instantiated static-member references (`n_template_args > 0`) still
  rely on the optimistic unresolved-template path after structured table misses;
  the delegated subset includes
  `cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp`
  as a guard for that compatibility route.
- A direct `build/tests/frontend/frontend_parser_tests` run is currently blocked
  by the existing failure:
  `FAIL: record-body using member typedef writer should register a direct record/member key`.
  The focused stale-owner authority check is covered in the passing
  `frontend_parser_lookup_authority_tests` binary instead.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 884/884
passing delegated positive Sema tests.

Additional check: `build/tests/frontend/frontend_parser_lookup_authority_tests`.
Result: passed, including the focused stale rendered-owner/member static-member
Sema coverage.

Additional check attempted: `build/tests/frontend/frontend_parser_tests`.
Result: failed at the existing record-body member-typedef writer assertion
noted above.
