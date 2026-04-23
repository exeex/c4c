Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Replace remaining suitable single-name string tables

# Current Packet

## Just Finished
Advanced Step 5 by migrating parser concept-name tracking to a `TextId`-first unqualified identity path: concept declarations now register unqualified `TextId`s in parser binding state, unqualified concept queries resolve through that native table first, and the legacy string set remains only as the namespace-qualified compatibility bridge.

## Suggested Next
Continue Step 5 with the next narrow single-name parser binding table that still uses `std::string` for semantic identity, again separating unqualified semantic storage from any required namespace-qualified bridge behavior.

## Watchouts
This packet intentionally preserved namespace-qualified concept lookup through `compatibility_namespace_name_in_context(...)` and the legacy string bridge. Do not collapse that bridge until the remaining qualified namespace consumers have a `TextId`-native replacement.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
Passed; proof log: `test_after.log`
