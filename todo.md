Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce structured qualified-name keys for parser-owned lookup

# Current Packet

## Just Finished
Migrated parser-owned `known_fn_names` tracking to structured qualified-name keys and routed the parser lookup/registration paths through the new structured helper instead of direct flat-string set access.

## Suggested Next
Migrate `ParserBindingState::struct_typedefs` to the same structured qualified-name key pattern so the remaining owner-scoped parser table stops relying on rendered strings.

## Watchouts
The structured key currently preserves existing behavior by keying off qualified spelling rather than namespace-context identity.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
Passed. Proof log: `test_after.log`
