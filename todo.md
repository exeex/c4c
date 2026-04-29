# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Convert Pure Parser Text Lookup To TextId

## Just Finished

Step 2 converted the function-local K&R parameter declaration lookup in
`parse_declaration_or_function()` from `std::unordered_map<std::string, Node*>`
to `std::unordered_map<TextId, Node*>`.

Identifier-list parameter names are interned once at the local lookup boundary,
K&R declaration names use the declarator-provided `TextId` with an interned
fallback for arena spellings, and `Node::name` remains the original spelling
used by the AST and diagnostics.

## Suggested Next

Next bounded conversion packet: pick the next pure parser-local string lookup
from the Step 1 inventory, likely record `field_names_seen`, and convert only
that local duplicate-name check to parser text identity if it has a clean
`TextId` source at the boundary.

## Watchouts

Do not widen the next packet into `struct_tag_def_map`, record layout
const-eval helpers, template rendered mirrors, public support helper
signatures, or files under `src/frontend/parser/impl/types/`.

For K&R parameters, declaration names without a usable spelling/text id are
ignored as before for malformed declarators; normal undeclared identifier-list
parameters still receive the old-style implicit `int` fallback.

## Proof

Proof command run exactly as delegated:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the frontend parser subset output.
