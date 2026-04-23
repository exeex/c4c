Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move unqualified visible lookup onto the new scope-local path

# Current Packet

## Just Finished
Completed the Step 4 parser packet by routing the remaining unqualified visible
typedef probes in statement/type helper code through the local-first facade.
`if`-condition declaration probing now checks the scoped visible typedef path
directly, and the qualified-type helper now treats unqualified visible typedefs
as present even when only the lexical scope state knows about them.

## Suggested Next
Continue Step 4 by sweeping the remaining parser helper call sites that still
use `resolve_visible_type_name(...)` as a pre-check for unqualified template or
struct type heads instead of consulting the scoped visible facade first.

## Watchouts
Do not collapse namespace traversal into lexical lookup. Step 3 only adds the
local-first facade; local bindings are still mirrored into the legacy flat
tables in some paths, so remaining Step 4 cleanup should keep qualified lookup
and namespace-owned resolution on their separate route.

## Proof
Proof command: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_parser_tests$'`
Result: passed; proof log at `test_after.log`.
