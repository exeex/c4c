Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Replace remaining suitable single-name string tables and isolate holdouts

# Current Packet

## Just Finished
Completed the first plan step 5 cleanup slice for concept lookup state.
Removed the touched `concept_names` string-set dependency, kept unqualified
concept discovery on `concept_name_text_ids`, and routed qualified concept
registration and lookup through the existing structured
`concept_qualified_names` path instead of bridge strings. Added parser unit
coverage for parser-owned unqualified concept probes and direct
namespace-qualified concept lookup.

## Suggested Next
Continue step 5 by auditing the remaining single-name lexical holdouts that
still keep bridge spelling alongside semantic identity, with template-scope
parameter name storage as the next likely packet if it still blocks
`TextId`-first lookup.

## Watchouts
Keep lexical concept visibility separate from namespace traversal. Do not
reopen the qualified-owner lookup slice completed under idea 84. Qualified
concept names should stay on structured namespace-context keys rather than
reintroducing rendered-name sets, and unqualified lexical concept checks
should keep using `TextId`-native identity for the touched parser paths.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parse_top_level_concept_decl_preserves_following_decl_dump)$'`
with proof captured in `test_after.log`.
