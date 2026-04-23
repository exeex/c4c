Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Replace remaining suitable single-name string tables

# Current Packet

## Just Finished
Advanced Step 5 by migrating parser enum-constant storage and helper lookup to `TextId` for unqualified enumerator identity, updating enum parsing to record `TextId` keys and constant-expression evaluation to resolve enum references through `unqualified_text_id`.

## Suggested Next
Continue Step 5 with the next narrow single-name parser binding table that still uses `std::string` for semantic identity, keeping qualified and namespace-scoped lookup paths out of scope.

## Watchouts
Keep enum work limited to unqualified enumerator identity. Do not fold qualified enum references, namespace traversal, or other spelling-dependent constant tables into the same packet.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
Passed; proof log: `test_after.log`
