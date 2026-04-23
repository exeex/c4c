Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by moving the shared visible value/type classifier
ahead of the constructor-init probe's unresolved `identifier identifier`
fallback. Safe visible-head cases now decide through
`classify_visible_value_or_type_head()` before the parser drops into the
legacy unresolved single-name special-case, which keeps the tentative
constructor-vs-function-declaration split on the shared lookup path where it
can.

## Suggested Next
Continue `plan.md` step 2 by auditing the remaining local
declaration/function-declaration ambiguity helpers for unresolved
single-name parameter-style starters that still need syntax-only
fallbacks, especially the grouped pointer/reference cases that have not yet
been folded into the shared visible-head lookup path.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes plus the statement-side tail
position checks that branch to expression parsing after a value-classified
head. The constructor-init probe still has a grouped pointer/reference
syntax fallback, and qualified call-like statement probes still rely on the
later `classify_visible_value_or_type_starter()` branch when the token after
the qualified head is `(` or a value-like template call.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`
passed. Proof log: `test_after.log`.
