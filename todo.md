Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by extending the declaration-side
`typedef` base-type fallback in `parser_declarations.cpp` to reuse the same
unresolved simple identifier type-head path that `parse_base_type()` already
uses in C++ mode. Top-level `typedef ForwardDecl Alias;`-style declarations
now register a placeholder typedef base instead of failing early before the
shared declaration/type-head logic can recover.

## Suggested Next
Continue `plan.md` step 2 by auditing the next declaration-side
identifier-as-type probe that still open-codes visible-type or placeholder
fallback instead of routing through the same shared helper family. Prefer the
next narrow packet in declaration parsing where a simple unqualified
identifier still gets classified with bespoke token-shape checks rather than
the parser-local lexical scope facade.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes; expression-side heuristics must
still distinguish value bindings from type bindings. The constructor-init
probe still rejects qualified and template-id starters with local token-shape
checks, so future retargeting should preserve those guards instead of
re-expanding the visible helper back into namespace traversal.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
