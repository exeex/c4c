# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete or Isolate Compatibility Bridge Helpers

## Just Finished

Step 4 audited and fenced declaration/type-struct owner-scope
`current_struct_tag` restoration paths. The local restore helpers in
`declarations.cpp` and `types/struct.cpp` no longer restore an invalid-TextId
rendered qualified fallback such as `ns::Owner` as active current-record owner
state, while structured unqualified owner TextIds are still restored.

Out-of-class operator/constructor/declarator owner-scope entry now installs
only a single unqualified structured owner TextId as `current_struct_tag`; a
multi-segment rendered owner spelling is cleared instead of becoming semantic
current-owner authority. Added parser tests for declaration owner-scope restore
and record-body context restore fail-closed behavior, plus structured restore
preservation.

## Suggested Next

Next packet: continue Step 4 by auditing the remaining parser
`current_struct_tag_text()` semantic consumers outside the now-fenced
declaration/type-struct restore paths, especially `typespec_matches_current_struct_local()`,
record special-member matching, and the type-helper current-record fallback
references.

## Watchouts

- In the scoped parser core/test audit, remaining rendered-spelling references
  are display rendering (`render_*`, `visible_name_spelling()`,
  `compatibility_spelling`) or fail-closed authority tests.
- `qualified_key_in_context()`, known-function/context registration,
  alias-template key interning, concept registration, and current-record member
  key construction now reject single rendered compound `TextId`/fallback inputs.
- `src/frontend/parser/impl/types/base.cpp` still has display/debug rendering
  fallback helpers, but the audited current-record member-typedef owner lookup
  no longer derives semantic owner identity from rendered spelling.
- `current_struct_tag_text()` remains in parser core/declarations/type-struct
  flows; the declaration/type-struct restoration paths are fenced, but
  same-spelling semantic consumers still need separate review before treating
  Step 4 as exhausted.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 356/356 tests passing.
