# Current Packet

Status: Active
Source Idea Path: ideas/open/519_rv64_object_emission_cleanup_umbrella.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Structure Baseline

## Just Finished

Step 1 - Establish Structure Baseline is complete. Created
`docs/rv64_object_emission_cleanup/structure_baseline.md` with the current
`object_emission.cpp` line count, clang-tool availability, AST query commands,
filtered symbol/function/type-reference evidence, initial region map, helper
families, and major dependency clusters.

## Suggested Next

Execute Step 2 from `plan.md`: compare the Step 1 RV64 region map against
`src/backend/mir/aarch64/codegen/` and existing RV64 destination candidates.
Produce an AArch64 comparison table and RV64 destination map draft in the
durable artifact directory.

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

## Proof

Analysis-only/no build per delegated proof. Commands/evidence are recorded in
`docs/rv64_object_emission_cleanup/structure_baseline.md`; no `test_after.log`
was produced or rewritten.
