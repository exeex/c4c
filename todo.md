Status: Active
Source Idea Path: ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Classify Responsibilities By First Owning Layer

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by classifying the inventoried `LIR -> BIR`
adapter responsibilities by first owning layer in
`docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`. The
note assigns the exposed responsibilities to LIR import, structured type/layout
bridge, initializer bridge, memory/address provenance import, call ABI import,
canonical BIR semantic model, or prepared/prealloc handoff, and compares that
split against BIR core cleanup, BIR/prealloc fusion, and RV64 post-contract
evidence.

## Suggested Next

Execute `plan.md` Step 3 by writing the durable handoff documents under
`docs/lir_bir_adapter_boundary/` using the Step 1 inventory and Step 2
ownership vocabulary. Keep the interface inventory, responsibility
classification, and ordered follow-up plan aligned on the same evidence
sources.

## Watchouts

- This active idea is umbrella triage; do not edit implementation files or
  treat direct backend case repair as progress.
- Treat ignored `build/` scan artifacts as evidence inputs only, not canonical
  lifecycle state.
- Keep `ValueMap`, `GlobalTypes`, `TypeDeclMap`, `FunctionSymbolSet`, local
  slot/pointer maps, structured layout fallback maps, CFG/phi scratch maps, and
  `memory_types.hpp` side tables import-local unless a later packet proves a
  narrower adapter contract.
- BIR route records and public query surfaces are canonical BIR semantic model
  authority; prepared plans, homes, frame/stack/call/storage products,
  carriers, wrappers, and MIR consumers stay prepared/prealloc or target
  handoff authority.
- RV64 post-contract evidence should route rows by first failing layer: exact
  `semantic lir_to_bir` rows remain adapter/BIR admission evidence, while
  prepared/module-shape and coherent RV64 object diagnostics are downstream
  boundary evidence.

## Proof

Passed: `git diff --check -- todo.md docs/lir_bir_adapter_boundary`.
No `test_after.log` is expected because this docs-only packet delegated a
direct diff-check proof and restricted root-level proof logs.
