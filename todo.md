Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by teaching the local ctor-init/function-declaration
probe to keep unresolved template-headed parameter forms such as
`Value<int>(other)` on the declaration-first path while still biasing known
visible value templates like `source<int>(payload)` toward
direct-initialization expressions. The probe now recognizes value-like
template starters backed by visible value or known function bindings instead
of letting them fall through to the generic parameter-list heuristic.

## Suggested Next
Continue `plan.md` step 2 by auditing the remaining ctor-init probe cases
where qualified starters such as `ns::value(...)` or `ns::value<T>(...)`
still rely on the generic parameter-list path instead of an explicit
value-vs-type split that stays separate from namespace traversal.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes; expression-side heuristics must
still distinguish value bindings from type bindings. The constructor-init
probe now special-cases unresolved `identifier identifier`,
`identifier &/* identifier`, simple parenthesized `identifier(...)`, and
grouped pointer/reference `identifier((...))` starters differently, and it now
also short-circuits value-like template starters only when the head resolves
through visible value or known-function lookup. Future cleanup should keep
that value-vs-type bias local to direct-init disambiguation instead of
re-expanding it into namespace-qualified lookup.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
