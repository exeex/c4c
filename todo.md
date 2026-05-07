# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete or Isolate Compatibility Bridge Helpers

## Just Finished

Step 4 audited the remaining rendered-qualified parser core/test references and
removed one leftover parser-core semantic reconstruction path:
`current_record_member_name_key()` no longer splits a rendered current-record
fallback such as `A::B` into structured member-key authority. It now accepts
only unqualified current-record TextId/name metadata.

Removed the dead rendered-qualified concept re-entry branch from
`is_concept_name()`. Added lookup-authority coverage proving rendered current
record spelling fails closed while unqualified current-record TextId metadata
still produces the structured member key.

## Suggested Next

Next packet: continue Step 4 by auditing parser type/declaration owner-scope
fallbacks that still derive current-record owner spelling from rendered names,
especially the `current_struct_tag` restoration and member-typedef lookup
callers outside parser core.

## Watchouts

- In the scoped parser core/test audit, remaining rendered-spelling references
  are display rendering (`render_*`, `visible_name_spelling()`,
  `compatibility_spelling`) or fail-closed authority tests.
- `qualified_key_in_context()`, known-function/context registration,
  alias-template key interning, concept registration, and current-record member
  key construction now reject single rendered compound `TextId`/fallback inputs.
- `src/frontend/parser/impl/types/base.cpp` still has non-owned
  `current_struct_tag_text()` fallback splitting near member typedef lookup; do
  not treat this packet as clearing that adjacent caller family.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 356/356 tests passing.
