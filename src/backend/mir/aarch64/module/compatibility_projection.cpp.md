# `src/backend/mir/aarch64/module/compatibility_projection.cpp` Replacement Draft

## Purpose

`compatibility_projection.cpp` owns migration-only public views derived after
canonical AArch64 MIR lowering completes. It is a Stage 3 replacement draft
artifact only. It does not describe the compiled legacy implementation, and it
does not authorize edits to real source, build, or test files.

The projection consumes completed canonical MIR, typed operands, prepared ABI
facts, and lightweight provenance:

`MachineModule + typed facts + provenance -> CompatibilityProjection`.

Compatibility projection never participates in semantic lowering decisions.
It does not decide instruction selection, operand storage precedence, branch
control, call ABI sequencing, or printer traversal.

## Classification

Compatibility projection.

This file is not module dispatch, function traversal, operand authority,
instruction lowering, branch/control lowering, call lowering, or public
assembly bridging.

## Projection Contract

The projection entry accepts the completed canonical module and read-only
sideband facts derived during lowering:

```cpp
CompatibilityProjection project_compatibility(
    const MachineModule& mir,
    const PreparedCompatibilityFacts& prepared_facts,
    const LoweringSidebandFacts& lowering_facts,
    ModuleLoweringDiagnostics& diagnostics);
```

The spelling above is illustrative. The required contract is that all public
compatibility records are derived from canonical MIR after lowering. Missing
projection data may produce an absent field or diagnostic; it must not cause
this seam to invent semantic lowering facts.

## `FunctionRecord::machine_nodes`

`FunctionRecord::machine_nodes` is a flat migration view beside canonical MIR.
It is populated by projecting each canonical `MachineFunction` in function,
block, and instruction order:

1. read the canonical function identity and label
2. read each canonical block identity, label, and successor metadata
3. read each canonical `MachineInstruction`
4. derive the legacy `codegen::InstructionRecord` shape from the typed opcode,
   typed operands, target render metadata, and provenance
5. attach projection diagnostics when a legacy field has no canonical
   equivalent

The flat vector is never the primary machine product and never feeds back into
lowering or printing. It is a temporary compatibility view for clients that
have not yet moved to `MachineFunction`, `MachineBlock`, and
`MachineInstruction`.

## Broad Inspection Records

Broad inspection records are derived views over completed lowering facts. They
may include:

- instruction inspection records
- branch and successor inspection records
- return inspection records
- call inspection records
- frame and stack inspection records
- spill/reload inspection records
- move and parallel-copy inspection records
- data and relocation inspection records
- diagnostic records explaining missing legacy projections

These records summarize canonical MIR and prepared facts already consumed by
the owning lowerers. They must not choose among broad optional fields to make
a narrow testcase pass, and they must not become a second source of truth for
lowering.

## Raw Source And Prepared Provenance

Compatibility may retain raw source or prepared views only as diagnostic
provenance with explicit lifetime and authority limits. Allowed retained
provenance includes:

- stable prepared IDs and indices
- durable source-location references
- labels and symbols already linked by prepared identity
- optional raw source/prepared pointers or views for diagnostics when lifetime
  is explicit and caller-owned lifetime is not required for canonical MIR
- reason tags that name the prepared authority used during lowering

Raw source/prepared views, cached display strings, and `std::string_view`
lifetime coupling are not replacement authority. They cannot repair missing
structured identity, register, operand, branch, call, or data facts.

## Label Views

Compatibility label views are derived from canonical function labels, block
labels, successor metadata, symbol operands, and relocation records. They may
provide legacy spelling fields when existing clients require them, but the
source is the canonical `LabelSymbol`, stable IDs, and prepared provenance.

Label projection must not recover block or function identity primarily from
source spellings, rendered assembly, cached strings, or flat machine-node
order.

## Legacy Register-String Diagnostics

Legacy register-string handling is diagnostic and fail-closed. Projection may
emit diagnostics that include old register spellings when:

- structured register facts are absent from legacy input evidence
- a compatibility client needs a human-readable explanation
- the diagnostic names the missing structured authority
- the diagnostic cannot override typed `MachineRegister` facts

Register-string fallback never produces semantic register authority for
operand resolution, instruction lowering, branch/control lowering, call
lowering, public assembly bridging, or canonical MIR construction.

## Removal Conditions

Compatibility views are removable when downstream clients consume canonical
MIR and typed facts directly. A view should have a tracked removal condition:

- `FunctionRecord::machine_nodes`: remove when clients read
  `MachineFunction`, `MachineBlock`, and `MachineInstruction` directly
- broad inspection records: remove when clients read typed lowering facts or
  canonical MIR plus provenance
- raw source/prepared diagnostic views: remove when structured provenance and
  diagnostics are sufficient
- label spelling views: remove when clients use `LabelSymbol` and stable IDs
- legacy register-string diagnostics: remove when clients report typed
  `MachineRegister` and `OperandAuthority` facts

No removal condition may require compatibility projection to become a semantic
lowering fallback first.

## Owned Inputs

- completed canonical `MachineModule`
- canonical `MachineFunction`, `MachineBlock`, and `MachineInstruction`
  carriers
- typed `MachineOperand`, `MachineRegister`, memory, label, symbol, immediate,
  and relocation-aware target forms
- prepared ABI facts already consumed by lowerers
- call-site ABI bindings, clobbers, preserved-value facts, indirect-callee
  facts, and memory-return facts for projection only
- function/block/value identity maps and durable source-location references
- private sideband facts produced by traversal, operand resolution,
  instruction lowering, branch/control lowering, and call lowering when those
  facts were derived from canonical lowering
- public legacy record type definitions needed to populate compatibility views

## Owned Outputs

- `CompatibilityProjection`
- `FunctionRecord` records
- derived flat `FunctionRecord::machine_nodes` vectors
- broad public inspection records
- raw source/prepared provenance views with explicit diagnostic limits
- label views derived from canonical labels and stable IDs
- legacy register-string diagnostics
- compatibility diagnostics for absent or lossy legacy fields
- removal-condition metadata or comments for each retained compatibility view

Owned outputs are migration products only. They are not inputs to future
semantic lowering decisions.

## Indirect Queries

Compatibility projection may indirectly query:

- canonical MIR functions, blocks, instructions, operands, successors, labels,
  symbols, and data records
- provenance attached to canonical MIR and module data
- lowering sideband facts that were produced while lowering canonical MIR
- prepared ABI and identity facts as explanatory provenance after lowering
- target render metadata only when deriving a legacy display field, not when
  choosing semantics

It must not query the public assembly bridge output, shared printer traversal,
prepared tables as alternate semantic authority, operand-resolution precedence
logic, instruction-selection internals, branch/control decision logic, call
sequencing internals, cached display strings as authority, or rendered
assembly as input.

## Forbidden Knowledge

`compatibility_projection.cpp` must not know:

- how to choose scalar, memory, move, spill/reload, parallel-copy, branch,
  return, or call instruction sequences
- how to choose operand storage precedence
- how to validate target-profile handoff as a lowering gate
- how to emit `.s` file structure or printer traversal
- how to repair incomplete canonical MIR from public records
- broad optional public records as semantic inputs
- cached display strings as instruction, operand, register, label, symbol,
  memory, immediate, data, relocation, branch, or call authority
- source spelling recovery as the primary identity model
- flat `FunctionRecord::machine_nodes` vectors as canonical output
- target render APIs named `__repr__`

## Upstream And Downstream Boundaries

Upstream lowerers may produce private sideband facts for projection, but those
facts must be derived during canonical lowering. Projection cannot ask an
upstream lowerer to rerun a semantic decision for compatibility.

The public assembly bridge and shared `mir_printer` consume canonical MIR, not
compatibility records. Compatibility may expose display-like fields for legacy
clients, but those fields are not the assembly-emission path.

## Replacement Rejection Signals

Reject an implementation of this draft if it:

- builds canonical MIR from `FunctionRecord::machine_nodes`
- asks compatibility records to decide instruction, branch, call, operand, or
  data semantics
- chooses among broad public record alternatives to satisfy a narrow testcase
- recovers missing structured facts from cached display strings or source
  spellings
- treats raw source/prepared object retention as the replacement authority
  model
- makes label views or register-string diagnostics authoritative
- feeds compatibility records into the public assembly bridge or shared
  printer
- adds target render APIs named `__repr__`
- omits removal conditions for retained migration views
