Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce structured qualified-name keys for parser-owned lookup

# Current Packet

## Just Finished
Migrated parser-owned `struct_typedefs` storage to structured qualified-name keys and routed the record-member registration paths through the structured owner/member helper instead of flat scoped-string keys.

## Suggested Next
Continue Step 2 with the next remaining parser-owned owner-scoped table, if one is still using a flat rendered-string fallback.

## Watchouts
The structured key preserves existing behavior by retaining the qualified owner spelling while still separating owner identity from the member name.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
Passed. Proof log: `test_after.log`
