# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete or Isolate Compatibility Bridge Helpers

## Just Finished

Step 4 fenced the parser type/base current-record member-typedef fallback:
`parse_base_type()` no longer splits `current_struct_tag_text()` spellings such
as `ns::Owner` to synthesize dependent member-typedef owner authority. The
fallback now requires an unqualified `current_struct_tag_text_id`; rendered
owner spelling remains display/no-complete-metadata compatibility only.

Added parser coverage proving a rendered-qualified current-record owner fails
closed for unqualified member typedef lookup while the structured owner TextId
path still resolves the dependent member typedef.

## Suggested Next

Next packet: continue Step 4 by auditing the remaining parser
type/declaration current-owner fallback references outside this
`parse_base_type()` member-typedef path, especially `current_struct_tag`
restoration in declaration/type-struct owner scopes.

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
  flows; review those owner-scope paths separately before treating Step 4 as
  exhausted.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 356/356 tests passing.
