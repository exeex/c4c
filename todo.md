Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting the unqualified visible-type probe
path onto parser-local `TextId` lexical scope state before the legacy visible
facade fallback. Added focused parser coverage that drives
`find_visible_typedef_type(TextId, ...)` through a scope-local alias and keeps
the visible-alias resolution behavior intact.

## Suggested Next
Continue `plan.md` step 2 with one narrow packet that audits the remaining
token-based visible-name predicates for unqualified type probes that still
route through bridge helpers instead of the parser-local `TextId` lexical
scope facade.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84, and do not treat
structured-qualified tables as candidates for flat `TextId` replacement.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`.
Proof log: `test_after.log`.
