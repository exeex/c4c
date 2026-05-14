# `src/backend/mir/aarch64/module/module.cpp` Replacement Draft

## Purpose

`module.cpp` owns the top-level AArch64 module dispatch from
`prepare::PreparedBirModule` into the public `BuildResult`. It is a Stage 3
replacement draft, not a Stage 1 extraction note and not a renamed copy of the
legacy broad record assembler.

The replacement route is:

`PreparedBirModule -> module dispatch -> function traversal -> canonical
MachineModule -> public Module -> CompatibilityProjection`.

Module dispatch validates the prepared handoff, coordinates module-level data
and relocation facts, invokes function traversal, and assembles the public
product from completed canonical MIR functions. It does not lower individual
operations and it does not use `FunctionRecord::machine_nodes` as the
replacement driver.

## Classification

Dispatch.

This file is the public build entry and module orchestrator. It is neither core
operation lowering, an optional fast path, public assembly bridging, nor
compatibility projection.

## Public Entry

The only public entry remains:

```cpp
[[nodiscard]] BuildResult build(const prepare::PreparedBirModule& prepared);
```

`build` returns a completed `Module` when target-profile resolution and the
AArch64 prepared-module handoff validate. It returns `abi::HandoffError` when
the prepared input cannot be accepted. It must not expose a partially lowered
record pile as a fallback product.

## Dispatch Flow

1. Resolve the target profile from `prepared`.
2. Validate that the prepared handoff is for AArch64 and is complete enough for
   direct prepared-BIR-to-MIR lowering.
3. Collect module-level data and relocation facts into `ModuleDataRecords` and
   the `MachineModule` data/relocation carrier.
4. Invoke function traversal for each prepared control-flow function, receiving
   completed `MachineFunction` carriers with ordered `MachineBlock` contents.
5. Build the canonical `MachineModule` from the completed function carriers and
   module data.
6. Build compatibility projections after canonical lowering completes.
7. Return `BuildResult{Module{target_profile, mir, data, compatibility},
   std::nullopt}`.

Failures before canonical lowering produce `BuildResult{std::nullopt, error}`.
Failures discovered by subordinate lowerers are reported through the same
handoff-error channel or a future typed module-lowering error shape; dispatch
must not paper over missing prepared authority by falling back to source
spellings or cached display strings.

## Target Profile Resolution

Target profile resolution is a module-level concern because the chosen profile
affects ABI validation, register classes, data hooks, and target-owned
rendering surfaces.

The dispatch layer may read:

- prepared target architecture and ABI profile records
- AArch64 ABI handoff metadata
- target data-layout facts already prepared for the module

It must fail closed when the profile is absent, non-AArch64, or inconsistent
with the prepared ABI facts. It must not infer the target from source file
names, rendered assembly strings, or legacy register spellings.

## AArch64 Prepared-Handoff Validation

Dispatch calls the AArch64 ABI handoff validator before creating the public
module product. Validation confirms that the prepared module carries the
minimum authority required by the downstream seams:

- prepared control-flow functions and block identities
- frame, storage, value-home, regalloc, spill/reload, parallel-copy, and call
  authority needed by traversal and lowerers
- module data, string, global, and relocation facts needed by data
  orchestration and the public assembly bridge
- target profile consistency for AArch64 operands, registers, and ABI classes

Validation is not instruction selection. It verifies that subordinate
components have authority to lower; it does not choose scalar ALU, branch,
spill/reload, return, or call instruction sequences.

## Module Data And Relocation Orchestration

Top-level data orchestration prepares canonical module data without becoming a
second lowering path. It owns:

- collecting globals and string constants from prepared module data
- preserving relocation needs in `MachineModule::relocation_needs` and
  `ModuleDataRecords`
- attaching lightweight `Provenance` reason tags for prepared module-data and
  relocation authorities
- passing data hooks to the public assembly bridge through typed module data

Data orchestration does not own common section traversal or `.s` file layout;
those belong to the shared `mir_printer` through the public assembly bridge.

## Public Product Assembly

`Module` assembly consumes only completed canonical MIR and module-level data:

```cpp
MachineModule mir {
  .functions = traverse_prepared_functions(...),
  .data_items = module_data_items,
  .relocation_needs = relocation_needs,
};

Module product {
  .target_profile = target_profile,
  .mir = std::move(mir),
  .data = data_records,
  .compatibility = project_compatibility(product.mir, prepared_facts),
};
```

The exact code shape can differ, but the dependency direction cannot:
compatibility projection runs after canonical `MachineModule` construction.
`FunctionRecord::machine_nodes` is a derived migration view, never a product
builder for `MachineFunction` or `MachineBlock`.

## Owned Inputs

- `prepare::PreparedBirModule`
- prepared target profile and AArch64 ABI handoff metadata
- prepared module data, globals, strings, and relocation facts
- prepared control-flow function list used to invoke function traversal
- subordinate component entry points for function traversal and compatibility
  projection
- shared ID namespaces and durable source-location references when already
  prepared

## Owned Outputs

- `BuildResult`
- public `Module`
- canonical `MachineModule`
- module-level `ModuleDataRecords`
- module-level data and relocation entries inside `MachineModule`
- top-level `Provenance` for module data and dispatch diagnostics
- compatibility projection assembled after canonical lowering

## Indirect Queries

Dispatch may ask subordinate seams for completed products:

- function traversal: completed `MachineFunction` carriers and any typed
  per-function sideband facts needed for later compatibility projection
- compatibility projection: migration records derived from canonical MIR and
  prepared ABI facts
- public assembly bridge: only through the final public module product, not as
  a lowering-time dependency

Dispatch must not query operand-resolution tables, operation-specific lowering
rules, branch fusion internals, call expansion details, or target assembly
syntax. Those are private to their owning seams.

## Forbidden Knowledge

`module.cpp` must not know:

- individual prepared operation lowering rules
- scalar ALU, memory, move, spill/reload, parallel-copy, branch, return, or
  call instruction selection details
- register spelling fallbacks as semantic authority
- source function or block recovery by spelling as the primary identity model
- broad inspection records as inputs to lowering
- public flat `FunctionRecord::machine_nodes` as a canonical carrier
- AArch64 assembly punctuation, spacing, or file structure
- cached display strings as instruction, operand, register, label, or symbol
  authority

## Provenance Rules

Dispatch may attach lightweight `Provenance` to the module and data records:
stable function/block/value IDs when relevant, prepared indices, durable source
locations, labels, and reason tags such as `PreparedTargetProfile`,
`PreparedModuleData`, `PreparedRelocationNeed`, or `ValidatedAArch64Handoff`.

It must not make raw prepared/source object ownership, `std::string_view`
lifetime coupling, or spelling recovery the replacement authority model. Raw
views may appear only in compatibility or diagnostic projections with explicit
lifetime notes.

## Replacement Rejection Signals

Reject an implementation of this draft if it:

- keeps the legacy broad `build_*_record` assembler as the replacement driver
- constructs public records first and asks later code to rediscover canonical
  MIR instructions from them
- treats `FunctionRecord::machine_nodes` or another flat vector as canonical
  output
- routes operation-specific lowering through module dispatch
- recovers missing prepared authority from strings instead of reporting a
  handoff/lowering error
- lets compatibility projection choose semantic lowering decisions
