# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete or Isolate Compatibility Bridge Helpers

## Just Finished

Step 4 audited the remaining parser `current_struct_tag_text()` consumers in
core visible-type resolution, expression operator-call owner collapse,
current-record member-typedef lookup, and record special-member matching.
Rendered qualified current-owner fallback strings such as `ns::Owner` no
longer authorize current-record visible-type matches, namespace-sibling type
fallback, expression owner collapse, or member-typedef record lookup; structured
unqualified current-owner TextIds still authorize the existing local current
record and sibling paths.

Added a lookup-authority regression for visible type resolution to document
that rendered qualified current-owner spelling fails closed while structured
unqualified owner TextId behavior remains intact. Record constructor/destructor
special-member matching was audited and did not require a code change because
identifier tokens cannot match a rendered qualified `current_struct_tag_text()`
such as `ns::Owner`.

## Suggested Next

Next packet: fence the out-of-owned `declarations.cpp`
`typespec_matches_current_struct_local()` direct rendered-string comparison
(`lhs == rhs`) so a rendered qualified current-owner fallback cannot match a
legacy rendered `TypeSpec` tag before structured visible-type resolution.

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
- The remaining direct `current_struct_tag_text()` references in owned files are
  fenced for rendered qualified owner spelling or are special-member token
  comparisons that fail closed for qualified display strings. `declarations.cpp`
  and `types/base.cpp` still contain out-of-packet references noted by earlier
  Step 4 work.
- Exact remaining blocker outside this packet's owned files:
  `src/frontend/parser/impl/declarations.cpp`
  `typespec_matches_current_struct_local()` still has a direct rendered
  `lhs == rhs` comparison before structured lookup authority.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 356/356 tests passing.
