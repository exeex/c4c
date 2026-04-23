Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Replace remaining suitable single-name string tables

# Current Packet

## Just Finished
Advanced Step 5 by migrating parser `const_int_bindings` to `TextId`-keyed unqualified identity, updating global constant binding registration to store declaration `TextId`s and constant-expression evaluation to resolve parser const-int references through unqualified `TextId` lookup only.

## Suggested Next
Continue Step 5 with the next narrow single-name parser binding table that still uses `std::string` for semantic identity, keeping qualified and namespace-scoped lookup paths out of scope.

## Watchouts
This packet intentionally left qualified const-int name behavior unchanged: parser constant-expression lookup now accepts only truly unqualified variable refs by `unqualified_text_id`. Do not fold namespace-qualified fallback behavior into the same migration.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
Passed; proof log: `test_after.log`
