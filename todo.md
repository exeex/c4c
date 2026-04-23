Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting the remaining constructor-vs-function
declaration probes in `parser_declarations.cpp`. Those helper paths now carry
the candidate argument token `TextId` through `resolve_visible_type_name(...)`
instead of rebuilding lookup from spelling before checking local visible type
bindings.

## Suggested Next
Continue `plan.md` step 2 with one narrow packet that audits parser
expression-side type/value disambiguation helpers for the same pattern,
especially the remaining `visible_type_head_name(...)` and visible-name
resolution call sites that still start from spelling when the current token
already carries a valid `TextId`.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables or post-parse enum/name fallbacks as candidates for
flat `TextId` replacement.

## Proof
Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`.
Proof log: `test_after.log`.
