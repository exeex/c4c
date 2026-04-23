Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by auditing the remaining identifier-head helper
sites used during template type/value disambiguation and simple type-head
probing. Retargeted those paths so they keep the current token `TextId` on the
local-visible-typedef-first lookup route instead of re-finding identifiers by
spelling before broader fallback logic.

## Suggested Next
Continue `plan.md` step 2 with one narrow packet that audits the remaining
template/declarator helper sites outside `parser_types_declarator.cpp` that
still rebuild identifier lookups from spelling rather than carrying the token
`TextId` through the local-visible scope facade.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables or post-parse enum/name fallbacks as candidates for
flat `TextId` replacement.

## Proof
Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`.
Proof log: `test_after.log`.
