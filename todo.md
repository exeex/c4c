Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting remaining parser-front-end
type-head probes onto the shared `TextId`-aware simple-type helper. The
`if`-condition declaration gate in `parser_statements.cpp` and the simple
template-argument fast path in `parser_types_declarator.cpp` now reuse the
same local-visible typedef/template/tag lookup path as the earlier local
declaration ambiguity probes.

## Suggested Next
Continue `plan.md` step 2 by auditing the remaining identifier-as-type
disambiguation checks in `parser_types_base.cpp` and nearby parser entry
paths, then retarget the next coherent manual probe to the same helper or to
the scope-local lookup facade where the helper is no longer sufficient.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes; expression-side heuristics must
still distinguish value bindings from type bindings. `parser_declarations.cpp`
continues to include `types_helpers.hpp` behind local renames for conflicting
helper bodies, so future retargeting there should avoid widening that include
surface unless the duplicated helpers are also reconciled.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
