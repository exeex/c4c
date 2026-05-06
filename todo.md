Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Parser Provisional Carrier Contract

# Current Packet

## Just Finished

Completed Step 2: Define Parser Provisional Carrier Contract. The parser
headers and support implementation now explicitly document the constant-layout
lookup carrier as provisional parser output: record kind through `TypeBase`,
tag `TextId`, namespace/global qualifier context, qualifier `TextId` sequence,
source spelling/location on the owning AST node, and declaration/reference role
from AST context. The contract also states that rendered strings remain
diagnostics/compatibility data, no new rendered-string semantic field is added,
and final record identity/completion belongs to Sema's record tables.

## Suggested Next

Delegate the next Sema record/table contract packet: make the Sema-owned
record-domain lookup/completion destination explicit for the constant-layout
family, including how parser `TypeSpec`/`Node` carrier metadata maps into the
record table without falling back to rendered-string identity.

## Watchouts

The current Sema key is structured but general-purpose; the next slice should
bound it as the interim record-domain key or introduce the missing
record-domain contract before changing parser constant-layout behavior.
`tag_text_id` alone must not become authoritative.

## Proof

Passed delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests)$'`.
Proof log: `test_after.log`.
