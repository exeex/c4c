Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by teaching typedef-chain helpers to re-probe
unqualified `ts.tag` names through the visible lexical facade before falling
back to flat typedef tables. Added focused parser coverage proving
`resolve_typedef_type_chain` now resolves a local alias target that exists only
in `ParserLexicalScopeState`.

## Suggested Next
Continue `plan.md` step 2 with one narrow packet that audits other helper-layer
typedef consumers for unqualified flat-table fallback after visible lookup,
especially template or struct-like resolution paths that still consult
`template_state_` or definition maps using resolved spelling without a final
lexical-scope-aware typedef probe.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables as candidates for flat `TextId` replacement.

## Proof
Built with `cmake --build --preset default` and proved the packet with
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`.
