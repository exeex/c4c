Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by teaching single-token template type-argument
disambiguation to accept visible lexical aliases through the visible typedef
facade instead of requiring only flat typedef-table membership. Added focused
parser coverage proving `try_parse_template_type_arg` now accepts a local alias
that resolves only through `ParserLexicalScopeState` plus visible alias lookup.

## Suggested Next
Continue `plan.md` step 2 with one narrow packet that audits other
template-disambiguation and parse-predicate helpers for unqualified checks that
still ask only `is_typedef_name(...)` or direct definition-map membership where
`has_visible_typedef_type(...)` should participate for lexical aliases.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables as candidates for flat `TextId` replacement.

## Proof
Built with `cmake --build --preset default` and proved the packet with
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`.
