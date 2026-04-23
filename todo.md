Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move unqualified visible lookup onto the new scope-local path

# Current Packet

## Just Finished
Completed another Step 4 parser packet by moving the remaining unqualified
template/helper typedef checks in `parser_types_template.cpp` and
`types_helpers.hpp` onto the scoped visible typedef facade. Template type-ref
decoding now probes visible typedef bindings before the flat table, and the
qualified-type helper marks unqualified typedef names as resolved when they
exist only in parser-local visible scope state while preserving the visible
typedef spelling instead of leaking the underlying tag.

## Suggested Next
Review whether any non-template parser helpers outside this packet still use
unqualified `has_typedef_type(...)` / `find_typedef_type(...)` checks where
the new local-first visible typedef facade should be consulted instead.

## Watchouts
Do not collapse namespace traversal into lexical lookup. Step 3 only adds the
local-first facade; local bindings are still mirrored into the legacy flat
tables in some paths, so remaining Step 4 cleanup should keep qualified lookup
and namespace-owned resolution on their separate route. The new helper is only
for unqualified visible typedef resolution; qualified owner/member traversal
should keep using its existing namespace and struct-member routes.

## Proof
Proof command: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_parser_tests$'`
Result: passed; proof log at `test_after.log`.
