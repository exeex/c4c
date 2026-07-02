# BIR Core Model Cleanup Umbrella

Status: Open
Type: Analysis umbrella idea
Order: after `ideas/closed/422_bir_semantic_producer_high_impact_cleanup.md`, before RV64 emission cleanup and other implementation ideas
Owning Layer: BIR core model and helpers
Primary Files:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_printer.cpp`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/bir/lir_to_bir/`

## Goal

Analyze the oversized BIR core files and produce a follow-up cleanup plan that
can split model declarations, helper algorithms, and route-specific analysis
without changing behavior in this umbrella.

## Why This Exists

`src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp` have become central
accumulation points for the BIR data model, derived semantic queries,
route-specific helper records, memory-access analysis, comparison producers,
select-chain helpers, and compatibility utilities.

This shape makes future BIR producer work harder to review because unrelated
model changes, helper extraction, and semantic route changes all appear in the
same files. After idea 422 selected follow-up BIR producer work, the next
maintenance step should be to make BIR ownership boundaries clearer before
more producer packets add to the same core files.

## In Scope

- Produce an inventory of top-level declaration families in `bir.hpp`.
- Produce an inventory of implementation families in `bir.cpp`.
- Use `.codex/skills/c4c-clang-tools/` for the initial C++ structure scan:
  confirm `c4c-clang-tool` and `c4c-clang-tool-ccdb` are available, then use
  AST-backed symbol, function-signature, caller/callee, and type-reference
  queries before falling back to raw long-file reading.
- Decide whether declarations should remain in one public model header with
  smaller private implementation files, move into existing focused files, or
  split into new headers by domain.
- Map likely destinations for:
  - core IR model types;
  - value/type/module/function/block/instruction definitions;
  - memory-address and object-storage records;
  - comparison and scalar producer helpers;
  - select-chain and direct-global dependency helpers;
  - route-specific memory-access analysis;
  - validation and printer support;
  - LIR-to-BIR-only helper surfaces.
- Identify include-cycle risks and public API compatibility constraints.
- Produce a staged follow-up cleanup plan with small implementation ideas.
- Preserve the ordering:
  `422 -> BIR cleanup analysis -> RV64 emission cleanup analysis -> other idea`.

## Out Of Scope

- Do not move declarations or definitions in this umbrella.
- Do not rename BIR types, values, opcodes, or route records.
- Do not change printer, validator, lowering, prepared, or RV64 behavior.
- Do not add new BIR semantic producer capability.
- Do not change gcc_torture expectations, allowlists, pass/fail accounting, or
  unsupported diagnostics.
- Do not fold idea 422 follow-up producer work into this cleanup analysis.

## Required Analysis Artifact

The runbook should produce a durable plan artifact under
`docs/bir_core_cleanup/` or `build/agent_state/518_bir_core_cleanup/` that
contains:

- current line-count and dependency snapshot;
- `c4c-clang-tools` query notes, including the commands used and the symbol or
  dependency clusters they exposed;
- declaration-family inventory for `bir.hpp`;
- implementation-family inventory for `bir.cpp`;
- recommended destination map;
- staged follow-up idea list;
- risk notes for public API, include churn, and validation scope.

## Follow-Up Plan Shape

The cleanup plan should prefer small behavior-preserving slices such as:

1. Move pure helper algorithms out of `bir.cpp` behind focused internal
   translation units.
2. Split route-specific analysis records away from the core data model when
   callers do not need the full surface.
3. Move printer-only and validator-only helpers toward their owning files.
4. Consider domain headers only where they reduce include pressure without
   creating circular dependencies.
5. Keep broad model type moves last, after low-risk helper extraction proves
   the include strategy.

## Acceptance Criteria

- The umbrella produces an explicit, reviewable BIR cleanup plan.
- The plan names concrete follow-up ideas and their owned files.
- The plan distinguishes existing-file redistribution from new-file splits.
- The plan records what should not be moved yet.
- No implementation files are changed except for lifecycle or documentation
  artifacts required by the analysis.
- The active plan remains unchanged unless this idea is explicitly activated
  later.

## Reviewer Reject Signals

Reject this umbrella or any claimed progress if the slice:

- moves BIR declarations or definitions while claiming to be analysis-only;
- changes BIR semantics, printer output, validation rules, prepared handoff, or
  RV64 lowering behavior;
- treats helper renames, expectation rewrites, unsupported downgrades, or
  pass/fail accounting changes as cleanup progress;
- folds idea 422 producer implementation work into file-organization planning;
- proposes one huge rewrite of `bir.hpp`/`bir.cpp` instead of staged,
  behavior-preserving follow-up ideas;
- retains the same monolithic ownership behind new filenames without defining
  clear caller and dependency boundaries;
- creates a plan that depends on testcase-shaped shortcuts or named
  gcc_torture cases rather than source-structure ownership.
