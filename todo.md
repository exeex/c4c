Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Sema Record/Tag Table Contract

# Current Packet

## Just Finished

Completed Step 3: Define Sema Record/Tag Table Contract. The Sema comments now
make the record-domain lookup destination explicit for constant-layout lookup:
record kind plus `SemaStructuredNameKey` metadata selects the record table,
typedef/value/function identity remains separate, forward declarations and
definitions converge through Sema record tables, and rendered-string mirrors are
compatibility, diagnostics, and testing bridges rather than semantic authority.

## Suggested Next

Delegate the first parser constant-layout migration packet: thread the parser
`TypeSpec` record-kind and structured name carrier into the Sema/HIR
constant-layout lookup path without adding new rendered-string authority.

## Watchouts

The contract is now explicit but behavior is unchanged. The next packet should
keep rendered-tag fallbacks as compatibility only and stop before parser
implementation edits if the migration needs a broader Sema/HIR table shape.

## Proof

Passed delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests)$'`.
Proof log: `test_after.log`.
