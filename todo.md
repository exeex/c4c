Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move unqualified visible lookup onto the new scope-local path

# Current Packet

## Just Finished
Completed another Step 4 parser packet by retargeting two remaining
unqualified helper paths away from flat typedef-table probes. Declaration-side
user-typedef conflict checks now consult the visible typedef facade for
single-name aliases, and record-body template-origin setup now avoids
synthesizing a flat typedef binding when an unqualified template-origin name
is already satisfied by scope-local visible lookup. Parser unit regressions
now prove both behaviors directly with local visible alias fixtures.

## Suggested Next
Continue Step 4 by reviewing the remaining namespace-resolution helpers in
`parser_core.cpp` that still fall back to direct `has_typedef_type(...)`
checks for unqualified names during visible type resolution. Keep the next
packet narrow: only retarget obviously unqualified fallback probes that can
use the visible facade without changing qualified or namespace-traversal
behavior.

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
