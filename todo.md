# Current Packet

Status: Active
Source Idea Path: ideas/open/519_rv64_object_emission_cleanup_umbrella.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Compare Against AArch64 Codegen Layout

## Just Finished

Step 2 - Compare Against AArch64 Codegen Layout is complete. Created
`docs/rv64_object_emission_cleanup/aarch64_comparison.md` with an AArch64
comparison table, RV64 destination map draft, existing RV64 owner notes,
intentional RV64/AArch64 divergences, and risk notes for symbol/fixup handling,
prepared admission, and runtime behavior.

## Suggested Next

Execute Step 3 from `plan.md`: draft staged behavior-preserving follow-up ideas
from `docs/rv64_object_emission_cleanup/structure_baseline.md` and
`docs/rv64_object_emission_cleanup/aarch64_comparison.md`. The next packet
should produce ordered follow-up slices with owned files, prerequisites,
validation expectations, and reviewer reject signals, without moving
implementation code.

## Watchouts

- This is an analysis umbrella; do not move RV64 implementation code in this
  plan.
- `prepared_function_to_object_function`, `fragment_for_prepared_instruction`,
  and `fragment_for_prepared_call` are high-coupling anchors with broad
  prepared/prealloc/BIR dependencies; do not treat them as early pure-helper
  moves.
- Symbol/fixup/data-object assembly is concentrated late in
  `object_emission.cpp` and should be compared carefully before proposing a
  destination.
- Keep F128/gcc_torture capability repair, expectation changes, and target-side
  inference out of this idea.
- Treat RV64 `calls.cpp`, `memory.cpp`, `globals.cpp`, `returns.cpp`,
  `prologue.cpp`, and `variadic.cpp` as historical layout references only until
  a later packet proves or creates live compiled ownership.
- Keep `prepared_function_to_object_function`,
  `fragment_for_prepared_instruction`, symbol/fixup module assembly, and
  prepared data-object emission as late or central boundaries; peel leaf helper
  families first.

## Proof

Analysis-only/no build per delegated proof. Commands/evidence are recorded in
`docs/rv64_object_emission_cleanup/aarch64_comparison.md`; no `test_after.log`
was produced or rewritten.
