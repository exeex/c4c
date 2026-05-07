# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete or Isolate Compatibility Bridge Helpers

## Just Finished

Step 4 deleted the parser rendered-qualified compatibility key reconstruction
helpers. `qualified_key_in_context()` now fails closed for any single rendered
compound `TextId` such as `A::B::C`, so semantic parser lookup helpers cannot
split the rendered spelling into structured key authority.

Added focused parser lookup-authority coverage proving rendered qualified
`TextId` registration/interner paths fail closed while unqualified
context-plus-name structured keys still register and resolve.

## Suggested Next

Next packet: continue Step 4 by auditing any remaining rendered qualified
spelling uses in parser core/tests and documenting or isolating them as
display-only/no-semantic-authority paths.

## Watchouts

- `find_compatibility_key_from_rendered_qualified_spelling()` and
  `intern_compatibility_key_from_rendered_qualified_spelling()` are gone from
  `src/frontend/parser/impl/core.cpp`; `rg` finds no remaining references in the
  scoped parser/test files.
- `qualified_key_in_context()` still builds structured keys from namespace
  context plus unqualified `TextId`; do not weaken that route while auditing
  display rendering.
- Any retained rendered spelling use should stay display-only or no-metadata
  compatibility outside semantic parser key construction.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 356/356 tests passing.
