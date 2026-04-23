Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by promoting the ctor-init starter classifier into a
shared parser helper and reusing it in block-statement decl-vs-expr routing for
qualified/global-qualified call-like heads. Global-qualified template calls now
stay on the expression path, and global/operator-owned qualified calls no
longer fall back to local-declaration parsing.

## Suggested Next
Continue `plan.md` step 2 by checking whether any remaining local
declaration/expression ambiguity probes still bypass the shared
`classify_visible_value_or_type_starter()` path, especially around
non-global operator-owned or scope-qualified expression heads that still rely
on token-shape shortcuts.

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
passed. Additional direct probes with `build/c4cll --parse-only` now show
`::api::source<int>(payload);` and `::BaseImpl::operator=(lhs, rhs);` as
expression statements. Proof log: `test_after.log`.
