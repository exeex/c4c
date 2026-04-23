Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by folding the local ctor-init probe onto a single
structured starter classifier that handles both qualified and unqualified
visible value/type heads before the generic parameter-list path. The direct
init probe now treats imported or global-qualified value starters as
expression-like when their structured value lookup resolves, while keeping
resolved type heads on the declaration side.

## Suggested Next
Continue `plan.md` step 2 by checking whether any remaining local
declaration/expression ambiguity probes can reuse the same structured
value-vs-type split for ctor-init starters without widening the lexical scope
path into namespace traversal.

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
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_cxx_)' | tee test_after.log` passed. Proof log: `test_after.log`.
