# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reprobe TypeSpec Tag Removal Build Boundary

## Just Finished

Completed Step 1 fixture migration for
`tests/frontend/cpp_hir_parser_core_typedef_chain_metadata_test.cpp`.

The remaining direct `TypeSpec::tag` fixture setup in this target now goes
through a SFINAE-gated helper. The stale rendered tag disagreement checks remain
active, and the `tag_text_id` typedef-chain assertions were not weakened.

Temporarily removed `const char* tag` from `TypeSpec` in
`src/frontend/parser/ast.hpp` and ran
`cmake --build build --target cpp_hir_parser_core_typedef_chain_metadata_test`.
The owned target built successfully with the field removed, so no remaining
deletion-probe blocker was found inside this owned target. Restored the field
before final proof, leaving no `ast.hpp` diff.

## Suggested Next

Run the supervisor-chosen next broad TypeSpec::tag deletion boundary check and
delegate the next fixture target if another direct test debt bucket appears.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Do not remove diagnostics, dumps, mangling, ABI/link spelling, or final
  output merely because they are string-shaped.
- Preserve stale-rendered-spelling disagreement tests.
- Split distinct downstream carrier boundaries into `ideas/open/*.md` instead
  of silently broadening the parent runbook.
- This owned test still has SFINAE helper references to `.tag` by design.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build --target cpp_hir_parser_core_typedef_chain_metadata_test c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_parser_core_typedef_chain_structured_metadata$'`

Result: passed. `cpp_hir_parser_core_typedef_chain_structured_metadata`
is green in `test_after.log`.

Controlled deletion probe:
- Temporarily removed `const char* tag` from `TypeSpec` in
  `src/frontend/parser/ast.hpp`.
- Ran
  `cmake --build build --target cpp_hir_parser_core_typedef_chain_metadata_test`;
  the focused target built successfully with the field removed, so no next
  blocker appeared inside this target.
- Restored the field and reran the delegated proof with the field present,
  leaving the repository buildable for the owned proof subset.
