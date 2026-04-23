Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting the remaining unqualified
identifier-as-type probes in `parser_types_base.cpp` onto a shared
`TextId`-aware visible-type helper. `is_type_start()` and the
`parse_base_type()` declaration/type-head gates now reuse one exact
typedef-or-template-parameter visibility predicate instead of carrying
separate duplicated checks.

## Suggested Next
Continue `plan.md` step 2 by auditing the remaining manual type-head
disambiguation paths that still bypass the shared helper, especially nearby
parser entry points that resolve unresolved simple identifiers into type
placeholders. Retarget only the next coherent declaration-side probe; do not
pull expression lookup or qualified-name traversal into this packet.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes; expression-side heuristics must
still distinguish value bindings from type bindings. `parser_declarations.cpp`
continues to include `types_helpers.hpp` behind local renames for conflicting
helper bodies, so future retargeting there should avoid widening that include
surface unless the duplicated helpers are also reconciled.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
