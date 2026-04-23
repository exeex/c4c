Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move unqualified visible lookup onto the new scope-local path

# Current Packet

## Just Finished
Started plan Step 4 by moving the remaining unqualified visible probes in
statement/helper logic onto the local-first lookup facade. The statement-side
type/value disambiguation now uses the scoped value lookup path, and the helper
type-head probe now asks the scoped typedef facade before falling back to the
legacy visible type spelling path.

## Suggested Next
Continue Step 4 by sweeping the remaining unqualified visible probes in parser
helper logic for any last manual resolve-plus-flat-table checks.

## Watchouts
Do not collapse namespace traversal into lexical lookup. Step 3 only adds the
local-first facade; local bindings are still mirrored into the legacy flat
tables until Step 4 finishes moving unqualified visibility onto the scoped
lookup path.

## Proof
Proof command: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_parser_tests$'`
Result: passed; proof log at `test_after.log`.
