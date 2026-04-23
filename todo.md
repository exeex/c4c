Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by teaching the parser's
parameter-type probe about grouped parenthesized declarators such as
`Value((*other))` while still biasing known visible values like
`source((*other))` toward direct-initialization expressions. The local
ctor-init/function-declaration probe now keeps these nested
pointer/reference declaration heads on the declaration-first path instead
of dropping back to token-shape-only value-expression heuristics.

## Suggested Next
Continue `plan.md` step 2 by auditing any remaining constructor-init probe
cases where unresolved identifier starters still depend on local token-shape
heuristics instead of visible lexical type-or-value bindings, especially
multi-token qualified or template-headed forms that should stay separate
from namespace traversal.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes; expression-side heuristics must
still distinguish value bindings from type bindings. The constructor-init
probe now special-cases unresolved `identifier identifier`,
`identifier &/* identifier`, simple parenthesized `identifier(...)`, and
grouped pointer/reference `identifier((...))` starters differently; future
cleanup should keep that value-vs-type bias local to direct-init
disambiguation instead of re-expanding it into namespace-qualified lookup.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
