Status: Active
Source Idea Path: ideas/open/129_parser_intermediate_carrier_boundary_labeling.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify parser helper structs

# Current Packet

## Just Finished

Step 2: Classified parser helper declarations in
`src/frontend/parser/parser_types.hpp` by boundary role using comment-only
updates: parser-local state/table, parse-time carrier, AST projection source,
cross-stage durable payload, diagnostics/debug carrier, and compatibility
spelling holder.

## Suggested Next

Delegate Step 3 to audit parser-side string authority in `parser.hpp`,
`parser_types.hpp`, and implementation references only as needed to classify
named fields, matching plan.md.

## Watchouts

- Step 2 was comment-only; declaration order, helper defaults, and behavior
  were preserved.
- Step 3 should distinguish local-only parser strings from generated-name-only
  spelling and suspicious cross-stage semantic authority without doing cleanup.

## Proof

`cmake --build --preset default > test_after.log 2>&1` completed
successfully. Proof log: `test_after.log`.
