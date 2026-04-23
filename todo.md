Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by teaching the visible typedef/value facades to
re-probe scope-local bindings after alias resolution returns an unqualified
target name. Added focused parser coverage proving `find_visible_typedef_type`
and `find_visible_var_type` now resolve `using`-alias targets that live only in
`ParserLexicalScopeState`.

## Suggested Next
Continue `plan.md` step 2 with one narrow packet that audits remaining
unqualified parser-visible probes for direct flat-table fallback after
name-resolution, especially any paths that resolve a visible name string and
then call `find_typedef_type` or `find_var_type` without a final local-scope
reprobe.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables as candidates for flat `TextId` replacement.

## Proof
Built with `cmake --build --preset default` and proved the packet with
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`.
