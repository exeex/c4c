Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Parser Record Lookup Families

# Current Packet

## Just Finished

Completed a Step 4 partial migration for parser constant-layout record lookup.
`resolve_record_type_spec` now keeps `record_def` as preferred authority and
rejects tag/TextId compatibility fallback when namespace/global/qualifier
metadata is present without `record_def`; the remaining fallback is bounded to
unstructured legacy/tag-only layout callers.

## Suggested Next

Continue Step 4 by migrating the next parser record lookup family that still
uses compatibility tag storage for structured carriers, keeping any fallback
limited to unstructured legacy callers.

## Watchouts

This slice intentionally does not add a Sema/HIR record-table handoff.
Structured parser carriers without `record_def` now fail parser-side
constant-layout lookup instead of recovering through stale tag storage.

## Proof

Passed delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests)$'`.
Proof log: `test_after.log`.
