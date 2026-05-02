# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Audit For Rename-Only Or Wrapper-Only Work

## Just Finished

Step 5 removed the parser `find_known_fn_name_key_from_spelling` /
`intern_known_fn_name_key_from_spelling` semantic wrapper route in
`src/frontend/parser/impl/core.cpp`. Semantic key construction from separate
`TextId` plus namespace-context carriers now goes through
`qualified_key_in_context`; the only retained rendered-string parsing is
visibly named `*_compatibility_key_from_rendered_qualified_spelling` and is
limited to no-segment-carrier TextIds whose text already contains `::`.
Parser tests were adjusted to build qualified keys from explicit
`QualifiedNameRef`/context carriers instead of `intern_semantic_name_key("a::b")`
when structured metadata is available.

## Suggested Next

Continue Step 5 with a review pass over any remaining rendered-qualified
parser compatibility call sites outside `core.cpp`, focusing on whether they
now have a real `QualifiedNameRef`, namespace context, or `QualifiedNameKey`
carrier that can replace the no-carrier compatibility bridge.

## Watchouts

- The retained no-carrier parser bridge is for single `TextId` values whose
  text already contains `::`; splitting those callers requires carrying
  qualifier segment TextIds before they reach `qualified_key_in_context`.
- `lookup_record_layout` remains a HIR-metadata/no-carrier blocker: removing
  the rendered `env.struct_defs->find(ts.tag)` lookup requires an env-carried
  structured HIR record owner/index or equivalent structured layout map outside
  this packet's owned Sema-only scope.
- Further HIR carrier work belongs under
  `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` before
  execution resumes on that boundary.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command after replacing the parser-side
`*_from_spelling` semantic wrapper route:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
