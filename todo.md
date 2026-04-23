Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by extracting
`classify_visible_value_or_type_head()` so constructor-style local
declaration probing can reuse the same visible value/type head classifier for
qualified names without requiring call-like tails. Local direct-init
disambiguation now short-circuits qualified visible member-access heads such
as `ns::payload.value` through the shared lexical/value lookup path instead of
falling back to a separate token-shape/template shortcut stack.

## Suggested Next
Continue `plan.md` step 2 by checking whether any remaining local
declaration/expression ambiguity probes still bypass the shared
visible value/type classification path, especially constructor-init or
statement probes that still re-parse qualified-name tails locally just to
recover the post-head token position for `.` / `->` / assignment handling.

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
`Box value(ns::payload.value);` on the direct-init declaration path while the
paired `Box copy(ns::Value(other));` case still parses as a function
declaration. Proof log: `test_after.log`.
