Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting the declaration-side
constructor-vs-function probe in `parser_declarations.cpp` onto the same
`TextId`-aware visible-type helper used by the updated type-head gates.
The single-argument direct-initialization probe now classifies simple
identifier arguments through one exact typedef-or-template-parameter
visibility predicate instead of the broader legacy helper.

## Suggested Next
Continue `plan.md` step 2 by auditing the next declaration-side
identifier-as-type disambiguation path that still bypasses the shared
visible-type helper, especially nearby parser entry points that turn
unresolved simple identifiers into placeholder type heads. Keep the packet
inside declaration parsing and leave expression lookup plus qualified-name
traversal untouched.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes; expression-side heuristics must
still distinguish value bindings from type bindings. The constructor-init
probe still rejects qualified and template-id starters with local token-shape
checks, so future retargeting should preserve those guards instead of
re-expanding the visible helper back into namespace traversal.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
