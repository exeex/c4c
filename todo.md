Status: Active
Source Idea Path: ideas/open/685_lir_import_context_extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Narrow One Import-Local State Family

# Current Packet

## Just Finished

Completed `plan.md` Step 3 CFG/phi scratch narrowing.

Changed files:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `todo.md`

The CFG/phi scratch declarations `BlockLookup`, `BranchChain`,
`PhiBlockPlanMap`, `PendingAggregatePhiCopy`, `PendingAggregatePhiCopyMap`,
`PendingScalarPhiProducer`, and `PendingScalarPhiProducerMap` are now private
to `BirFunctionLowerer` instead of sitting in its public nested declaration
surface. `module.cpp` was inspected with clang tooling but did not need a
retained source edit.

Public adapter entry behavior is untouched: no public entry declarations,
options, result envelope, diagnostics, tests, expectations, unsupported
markers, allowlists, or public adapter files changed.

## Suggested Next

Next coherent packet: have the supervisor select the next import-local state
family from the Step 1 inventory, preserving the Step 2 public entry behavior
boundary and avoiding structured layout, initializer, memory/provenance, and
call ABI state unless explicitly delegated.

## Watchouts

`PhiLoweringPlan` remains public because the existing anonymous-namespace
helper `merge_runtime_pointer_phi_address` in `module.cpp` names
`BirFunctionLowerer::PhiLoweringPlan`; moving it private would require a wider
helper ownership/signature change. `cfg.cpp` also owns split-TU member
definitions for CFG/phi helpers, but the C++ member definition model allowed
`BlockLookup`, `BranchChain`, and `PhiBlockPlanMap` to move private without
touching that unowned file.

## Proof

Delegated proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

`test_after.log` reports `100% tests passed, 0 tests failed out of 302`.
