# `src/backend/mir/aarch64/module/public_assembly_bridge.cpp` Replacement Draft

## Purpose

`public_assembly_bridge.cpp` owns the bridge from the completed public AArch64
`Module` product to the shared `mir_printer`. It is a Stage 3 replacement
draft artifact only. It does not describe the compiled legacy implementation,
and it does not authorize edits to real source, build, or test files.

The bridge consumes canonical MIR and module data after lowering has completed:

`Module{MachineModule, ModuleDataRecords} -> mir_printer -> .s text`.

The shared printer owns traversal and common emission mechanics. AArch64 owns
only target-specific rendering hooks for instructions, operands, registers,
memory references, labels, symbols, immediates, relocation-aware forms, and
target data fragments.

## Classification

Public assembly bridging.

This file is not module dispatch, function traversal, operand authority,
instruction lowering, branch/control lowering, call lowering, or compatibility
projection. It does not participate in semantic lowering decisions.

## Bridge Contract

The bridge entry accepts a completed public module product and an output sink:

```cpp
void emit_public_assembly(
    const Module& module,
    mir_printer::AssemblySink& sink,
    ModuleLoweringDiagnostics& diagnostics);
```

The spelling above is illustrative. The required contract is that the bridge
adapts AArch64 target render hooks and module data hooks to one common
`mir_printer` traversal. The bridge must not traverse functions, blocks, flat
machine-node vectors, or compatibility records itself to create `.s` file
structure.

## Canonical Input

The bridge consumes only completed canonical and public module data:

- `Module::target_profile`
- `Module::mir`
- `MachineModule::functions`
- `MachineFunction`, `MachineBlock`, and ordered `MachineInstruction` nodes
- `MachineModule::data_items` and relocation needs
- `ModuleDataRecords` for public data metadata
- target-owned render surfaces declared by the public header draft

The bridge is downstream of lowering. It must not ask prepared BIR tables,
function traversal, operand resolution, instruction lowering, branch/control
lowering, call lowering, or compatibility projection to complete missing
semantic facts.

## Shared `mir_printer` Ownership

The common `mir_printer` owns:

- module traversal
- function order and function label placement
- block order and block label placement
- instruction scan order inside each block
- section selection and section transitions
- common spacing and line emission mechanics
- data traversal and relocation emission structure
- `.s` file prologue, body, and epilogue structure

The bridge calls into the shared printer with canonical MIR containers and
target hooks. AArch64 code must not duplicate common traversal or emit an
independent assembly file by walking the module itself.

## AArch64 Rendering Hooks

AArch64 owns the concrete render hooks called by the shared printer:

```cpp
struct AArch64RenderHooks {
  void render_instruction(const MachineInstruction& instruction,
                          mir_printer::TargetRenderSink& sink) const;
  void render_operand(const MachineOperand& operand,
                      mir_printer::TargetRenderSink& sink) const;
  void render_register(const MachineRegister& reg,
                       mir_printer::TargetRenderSink& sink) const;
  void render_memory(const MemoryReference& memory,
                     mir_printer::TargetRenderSink& sink) const;
  void render_symbol(const GlobalSymbol& symbol,
                     mir_printer::TargetRenderSink& sink) const;
  void render_label(const LabelSymbol& label,
                    mir_printer::TargetRenderSink& sink) const;
  void render_immediate(const ImmediateValue& immediate,
                        mir_printer::TargetRenderSink& sink) const;
};
```

The exact C++ spelling can differ, but instruction rendering and operand
rendering remain separate target-owned surfaces. Target-owned APIs must use
ordinary C++ names and must not introduce a C++ method or public API named
`__repr__`.

## `.s` Emission Through Common Traversal

Assembly emission follows this dependency direction:

1. Module dispatch returns a completed `Module`.
2. The public bridge prepares AArch64 render hooks and data hooks.
3. The bridge invokes the shared `mir_printer` once with `module.mir` and the
   hooks.
4. The shared printer traverses module data, functions, blocks, and
   instructions in canonical order.
5. The shared printer asks AArch64 hooks only for target text fragments.

The bridge may validate that the public module is complete enough to print.
Validation failures are diagnostics. They are not permission to rebuild
machine instructions from prepared tables, cached display strings, broad
public inspection records, or compatibility projections.

## Module Data Hooks

Module data and relocations are canonical public products, not a second
lowering path. The bridge may provide target hooks for:

- global object labels
- string object contents
- target data directives
- relocation-aware symbol references
- target alignment directives when already represented by module data facts
- data provenance diagnostics

The common printer still owns section ordering and file structure. AArch64
hooks render target-specific data fragments only when the printer requests
them.

## Owned Inputs

- completed public `Module`
- canonical `MachineModule`
- canonical `MachineFunction`, `MachineBlock`, and `MachineInstruction`
  carriers
- typed `MachineOperand`, `MachineRegister`, memory, label, symbol, immediate,
  and relocation-aware target forms
- `ModuleDataRecords`, `ModuleDataItem`, and relocation-needs records
- target profile render constraints for AArch64
- shared `mir_printer` traversal and sink interfaces
- lightweight provenance already attached to canonical MIR and module data for
  diagnostics

## Owned Outputs

- `.s` assembly text emitted through the shared `mir_printer`
- AArch64 target render hooks supplied to the shared printer
- target data render hooks supplied to the shared printer
- diagnostics for incomplete canonical MIR, unrenderable typed operands, or
  unsupported target data forms

The bridge does not own compatibility records, public inspection records, or
flat machine-node vectors as output products.

## Indirect Queries

The bridge may indirectly query:

- target render hooks on canonical AArch64 machine instructions and operands
- target profile metadata needed to render already-lowered target forms
- module data hooks for already-lowered global, string, and relocation records
- provenance attached to canonical MIR for diagnostics
- the shared `mir_printer` for common traversal and emission services

It must not query prepared BIR lowering authority, operand-resolution storage
precedence, instruction-selection internals, branch/control lowering
internals, call-plan internals, compatibility projection records, broad public
inspection records, cached display strings, source spellings, or
`FunctionRecord::machine_nodes` while deciding what assembly to emit.

## Forbidden Knowledge

`public_assembly_bridge.cpp` must not know:

- semantic instruction, branch, return, call, spill/reload, or parallel-copy
  lowering rules
- prepared value homes, storage plans, regalloc assignments, call plans, or
  branch terminator authority as semantic inputs
- function, block, instruction, data, or section traversal algorithms
- common `.s` file structure beyond invoking the shared printer
- compatibility projection layout
- broad public inspection records as print authority
- flat `FunctionRecord::machine_nodes` vectors as print authority
- cached display strings as instruction, operand, register, label, symbol,
  memory, immediate, data, or relocation authority
- source spelling recovery as the identity model
- target render APIs named `__repr__`

## Downstream Use

External callers receive assembly only through the public module product and
the shared printer. Compatibility projection may expose migration records for
legacy clients, but the printer bridge must not read those records and must not
ask compatibility projection to synthesize missing printable nodes.

## Replacement Rejection Signals

Reject an implementation of this draft if it:

- traverses functions, blocks, instructions, sections, or data in AArch64 code
  instead of delegating traversal to the shared `mir_printer`
- emits `.s` file structure directly from AArch64 module code
- prints from `FunctionRecord::machine_nodes`, broad public inspection
  records, or compatibility projections
- uses cached display strings or source spellings as target print authority
- re-walks prepared lowering tables to repair incomplete canonical MIR
- combines instruction and operand rendering into an untyped cached string
- introduces target render APIs named `__repr__`
- treats the public bridge as a semantic lowering participant
