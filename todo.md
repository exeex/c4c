Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move unqualified visible lookup onto the new scope-local path

# Current Packet

## Just Finished
Completed the Step 4 parser packet by routing the remaining unqualified visible
typedef probes in declarator/template parsing through the local-first facade.
The dependent-typename branch now checks the scoped visible typedef path before
falling back to legacy visible spelling resolution, and the template member
lookup fallback now consults the scoped visible typedef facade before resolving
the legacy visible template spelling.

## Suggested Next
Continue Step 4 by sweeping any remaining parser helper call sites that still
resolve an unqualified visible name manually before checking the flat typedef
tables.

## Watchouts
Do not collapse namespace traversal into lexical lookup. Step 3 only adds the
local-first facade; local bindings are still mirrored into the legacy flat
tables until Step 4 finishes moving unqualified visibility onto the scoped
lookup path.

## Proof
Proof command: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_parser_tests$'`
Result: passed; proof log at `test_after.log`.
