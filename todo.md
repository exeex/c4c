Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move unqualified visible lookup onto the new scope-local path

# Current Packet

## Just Finished
Completed the Step 4 packet by retargeting the remaining unqualified visible
type fallback probes in `parser_core.cpp` away from direct flat
`has_typedef_type(...)` checks. Unqualified value-alias resolution now probes
the scope-local visible typedef path before falling back, while qualified
alias targets keep the existing namespace-qualified route. Parser regressions
now prove both the scope-local case and the qualified boundary case.

## Suggested Next
Continue Step 4 by reviewing the remaining parser helpers that still rely on
flat typedef-table probes for unqualified visible lookup. Keep the next packet
narrow: only retarget checks that can use the visible facade without changing
qualified lookup or namespace traversal.

## Watchouts
Do not collapse namespace traversal into lexical lookup. This packet only
retargets unqualified visible-type probes to the scope-local facade; qualified
names, owner/member traversal, and composed namespace lookup still need to
stay on their existing routes. Keep `A::B`-style lookups on the
namespace/qualified path.

## Proof
Proof command: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_parser_tests$'`
Result: passed; proof log at `test_after.log`.
