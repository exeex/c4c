Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by auditing the remaining local declaration
ambiguity probes in `parser_declarations.cpp`. The constructor-vs-function
paths now use the shared `TextId`-aware simple-type helper instead of the
remaining spelling-first visible-type/tag checks.

## Suggested Next
Continue `plan.md` step 2 by checking whether any other local declaration
branches in `parser_declarations.cpp` still need the same shared helper
retargeting.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe because the TU now includes `types_helpers.hpp` behind local
renames for the conflicting helper bodies, so future edits in this file should
avoid widening that include surface unless the duplicated helpers are also
reconciled.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
