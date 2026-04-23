Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Introduce a unified TextId-first lookup facade

# Current Packet

## Just Finished
Completed plan Step 3 by adding `TextId`-aware visible lookup helpers that
probe the new lexical typedef/value tables first for unqualified names, then
fall back to the existing namespace and compatibility bridge paths.

## Suggested Next
Start Step 4 by routing more unqualified visible lookup call sites through the
new local-first facade so block-local visibility stops depending on the legacy
flat parser binding tables.

## Watchouts
Do not collapse namespace traversal into lexical lookup. Step 3 only adds the
local-first facade; local bindings are still mirrored into the legacy flat
tables until Step 4 finishes moving unqualified visibility onto the scoped
lookup path.

## Proof
Proof command: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_parser_tests$'`
Result: passed; proof log at `test_after.log`.
