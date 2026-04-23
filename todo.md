Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by retargeting the constructor-vs-function-decl
probe in `parser_declarations.cpp`. The unqualified visible-type checks in the
local declaration ambiguity probe now use the shared `TextId`-aware
`visible_type_head_name(...)` helper instead of spelling-first visible-type
resolution.

## Suggested Next
Continue `plan.md` step 2 by auditing the remaining unqualified type-discovery
branches in `parser_declarations.cpp` for any other spelling-first visible-type
checks that should switch to the shared helper.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe because the TU now includes `types_helpers.hpp` behind local
renames for the conflicting helper bodies, so future edits in this file should
avoid widening that include surface unless the duplicated helpers are also
reconciled.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
