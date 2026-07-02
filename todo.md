# Current Packet

Status: Active
Source Idea Path: ideas/open/519_rv64_object_emission_cleanup_umbrella.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Draft Staged Cleanup Follow-Ups

## Just Finished

Step 3 - Draft Staged Cleanup Follow-Ups is complete. Created
`docs/rv64_object_emission_cleanup/staged_followups.md` with an ordered
behavior-preserving cleanup list sourced from the Step 1 structure baseline and
Step 2 AArch64 comparison. Each proposed slice records owned files,
prerequisites/dependencies, risks, validation expectations, and reviewer reject
signals, with symbol/fixup/object-module assembly marked as a late central
boundary.

## Suggested Next

Execute Step 4 from `plan.md`: materialize concrete `ideas/open/*.md`
follow-up source ideas from
`docs/rv64_object_emission_cleanup/staged_followups.md`. Start with a narrow
low-risk slice such as shared encoding/byte append helpers or pure frame/stack
offset helpers, and keep late symbol/fixup/object-module assembly as an
explicit later boundary.

## Watchouts

- This is an analysis umbrella; do not move RV64 implementation code in this
  plan.
- Step 4 should create source ideas only; do not start implementation movement
  in the same packet.
- Keep F128/gcc_torture capability repair, expectation changes, unsupported
  marker changes, and target-side inference out of the materialized ideas.
- Treat RV64 `calls.cpp`, `memory.cpp`, `globals.cpp`, `returns.cpp`,
  `prologue.cpp`, and `variadic.cpp` as historical layout references unless a
  materialized idea explicitly creates or proves live compiled ownership.
- Keep `prepared_function_to_object_function`,
  `fragment_for_prepared_instruction`, symbol/fixup module assembly, and
  prepared data-object emission as late or central boundaries.
- Do not materialize one broad catch-all cleanup idea that hides monolithic
  coupling behind new filenames.

## Proof

Analysis-only/no build per delegated proof. Evidence sources are recorded in
`docs/rv64_object_emission_cleanup/staged_followups.md`; no `test_after.log`
was produced or rewritten.
