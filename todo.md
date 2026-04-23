Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move unqualified visible lookup onto the new scope-local path

# Current Packet

## Just Finished
Completed another Step 4 parser packet by retargeting remaining unqualified
owner/member typedef reconstruction helpers through the parser-visible scope
facade instead of direct legacy typedef-table probes. Struct-like member
typedef lookup in `parse_base_type()`, dependent owner-tag recovery in
`parse_dependent_typename_specifier(...)`, and template-arg type-ref decoding
now consult visible typedef lookup first for unqualified aliases, and a parser
unit regression now proves that `Alias::type` resolves through a scope-local
owner alias.

## Suggested Next
Continue Step 4 by reviewing the remaining unqualified parser helpers that
still call `has_typedef_type(...)` or `find_typedef_type(...)` directly
outside the newly patched member-type reconstruction path, especially record
and declaration helpers that still probe unqualified aliases through the flat
typedef tables.

## Watchouts
Do not collapse namespace traversal into lexical lookup. This packet only
retargets obviously unqualified typedef probes to the visible-scope facade;
qualified names, owner/member traversal, and composed namespace lookup still
need to stay on their existing routes. Remaining Step 4 work should keep
using the visible facade only when the semantic name is truly unqualified and
leave `A::B`-style lookups on the namespace/qualified path.

## Proof
Proof command: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_parser_tests$'`
Result: passed; proof log at `test_after.log`.
