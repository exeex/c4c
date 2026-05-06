Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Parser Record Lookup Families

# Current Packet

## Just Finished

Completed the Step 4 repair for parser constant-layout record lookup.
`resolve_record_type_spec` still prefers `record_def` and rejects stale
structured tag fallback, but now accepts structured no-`record_def` carriers
when the record table has a matching full context or one unambiguous structured
record candidate. This restores `cpp_eastl_vector_parse_recipe` without using
rendered/tag fallback as record authority.

## Suggested Next

Continue Step 4 by migrating the next parser record lookup family that still
uses compatibility tag storage for structured carriers, keeping fallback paths
bounded by structured identity or legacy-only compatibility.

## Watchouts

Context-defaulted structured record carriers are accepted only when the parser
record table has one unique structured candidate for the TextId; ambiguous
matches still fail so `tag_text_id` alone does not choose between records.

## Proof

Passed delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_eastl_vector_parse_recipe)$'`.
Proof log: `test_after.log`.
