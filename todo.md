Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting deferred template-expression helpers
onto the `TextId`-first visible typedef facade when probing single-token
template arguments and template-owner aliases. Added focused parser coverage
proving deferred NTTP builtin-trait evaluation and `Trait<...>::member`
instantiation both accept scope-local aliases bound only through
`ParserLexicalScopeState`, and refreshed the local alias template-arg
expectation to match the existing resolved-type result.

## Suggested Next
Continue `plan.md` step 2 with one narrow packet that audits remaining
token-based parser predicates for unqualified visible-type probes that still
route through string-only overloads or pre-`TextId` bridge helpers instead of
passing the token `text_id` directly into the visible lexical facade.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables as candidates for flat `TextId` replacement.

## Proof
Built with `cmake --build --preset default` and proved the packet with
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`.
