Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by routing block-statement visible-name probes
through the shared `classify_visible_value_or_type_starter()` helper for
assignment/member-access disambiguation too, instead of keeping separate local
value-binding heuristics in `parse_stmt()`. Qualified visible-value member
access now stays on the expression path under the same lexical/value
classifier used for call-like heads.

## Suggested Next
Continue `plan.md` step 2 by checking whether any remaining local
declaration/expression ambiguity probes still bypass the shared
`classify_visible_value_or_type_starter()` path, especially around
template/member-expression heads that still rely on token-shape shortcuts
instead of the shared visible value/type classifier.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes; expression-side heuristics must
still distinguish value bindings from type bindings. The constructor-init
probe still special-cases unresolved `identifier identifier`,
`identifier &/* identifier`, simple parenthesized `identifier(...)`, and
grouped pointer/reference `identifier((...))` starters differently, and it
now short-circuits both unqualified and qualified value-like starters only
when the head resolves through visible value, known-function, or visible
type lookup. Future cleanup should keep that value-vs-type bias local to
direct-init disambiguation instead of re-expanding it into namespace-qualified
lookup.

## Proof
`cmake --build --preset default` passed, and
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`
passed. Added a focused frontend parser regression that keeps
`api::payload.value` on the expression path inside `main`, alongside the
existing global-qualified template-call and operator-call statement
disambiguation coverage. Proof log: `test_after.log`.
