# `src/backend/mir/aarch64/module/module.cpp` Extraction

## Purpose And Current Responsibility

`module.cpp` builds the AArch64 MIR module snapshot from
`prepare::PreparedBirModule`. It is a broad adapter, not a clean domain owner:
it validates the prepared AArch64 handoff, merges many prepared analysis tables,
normalizes register names and stack locations, preserves source pointers, and
packages the result into the record types declared by `module.hpp`.

The public surface is intentionally small:

```cpp
BuildResult build(const prepare::PreparedBirModule& prepared);
```

The produced `Module` is mostly a read-only overlay over prepared state. Many
records retain pointers back into `prepared`, so the returned module depends on
the caller keeping the prepared module alive.

## Important APIs And Contract Surfaces

### Public Entry

`build` is the only exported function in this implementation file. Its contract:

- resolve the target profile from prepared state
- run AArch64 handoff validation before producing a module
- build global, string, relocation, and function records
- return either `{module, null error}` or `{null module, error}`

```cpp
BuildResult build(const prepare::PreparedBirModule& prepared);
```

### Internal Record Builders

The internal API surface is a chain of `build_*_record(s)` helpers. These are
not independent passes; they are adapters over prepared tables and are ordered
by `build_function_record`.

```cpp
FunctionRecord build_function_record(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedControlFlowFunction& function);

std::vector<FunctionRecord> build_function_records(
    const prepare::PreparedBirModule& prepared);
```

High-value internal builders:

- operand merge: value homes, regalloc values, and storage-plan values into
  `OperandRecord`
- frame records: frame plan, stack layout slots, callee saves, dynamic stack
  operations
- call records: call arguments, results, preserved values, clobbers, indirect
  callees, memory returns
- move records: value-location moves, ABI bindings, regalloc move resolution
- spill/reload records and provisional spill/reload machine nodes
- return-related scalar ALU and return machine nodes
- parallel-copy records and their resolved move-step targets
- global/string data records and relocation needs

### Register And Storage Contracts

Several helpers normalize prepared register representations into AArch64 ABI
records. They accept both newer structured placements and legacy register-name
strings:

```cpp
abi::PreparedRegisterConversionResult convert_prepared_register_reference(
    const std::optional<prepare::PreparedRegisterPlacement>& placement,
    const std::optional<std::string>& legacy_register_name,
    prepare::PreparedRegisterBank bank,
    prepare::PreparedRegisterClass reg_class,
    std::optional<abi::RegisterView> expected_view = std::nullopt);
```

Register records also carry allocation authority and snapshot kind:

- `ValueHome`
- `RegallocAssignment`
- `SpillAuthority`
- `StoragePlan`

This file decides how those authorities are reflected in `TargetRegisterRecord`
and `OperandRecord`, but it does not own allocation itself.

## Dependency Direction And Hidden Inputs

Primary dependency direction:

`prepare::PreparedBirModule` -> AArch64 ABI validation/conversion -> module
records -> limited `codegen::InstructionRecord` nodes -> generic MIR container.

Important direct dependencies:

- `prepare`: name resolution, prepared locations, storage plans, call plans,
  frame plans, dynamic stack plans, control flow, regalloc, parallel copies
- `bir`: source functions, blocks, values, globals, string constants,
  terminators, binary instructions
- `abi`: target profile, handoff validation, register parsing/conversion,
  register pools, reserved/long-lived register classification
- `codegen`: operand and instruction-record constructors for scalar ALU,
  spill/reload, return, and MIR conversion
- `mir`: generic machine node/function/block containers

Hidden inputs and assumptions:

- Name identity is recovered through both structured IDs and legacy spellings.
  `find_source_function` and `find_source_block` fall back across link names,
  block labels, and prepared-name lookup.
- The returned module stores source pointers into `prepared`, so lifetime is
  external and implicit.
- Prepared stack offsets are treated as authoritative snapshots when present.
- Register-name strings may still be needed when structured placements are
  absent or conversion fails.
- Some machine nodes are record-only and intentionally provisional, not final
  lowering.
- `prepared.control_flow.functions` is the driver list for functions; source
  BIR functions are searched only to enrich records and synthesize limited
  machine nodes.

## Responsibility Buckets

### 1. Prepared-State Lookup And Normalization

The file contains many small lookup helpers for source functions/blocks,
regalloc values, frame slots, stack objects, storage-plan values, destination
slots, and prepared labels. These helpers compensate for prepared data being
split across independent tables.

### 2. Allocation And Operand Snapshot Assembly

`build_operands` merges value homes, regalloc assignments, and storage-plan
values. Precedence is partly encoded by mutation order: value homes first,
regalloc second, storage plan third. The record is a synthesized view with
authority labels rather than a single source of truth.

### 3. Frame, Stack, And Call Surface Assembly

Frame construction joins frame plans, stack layout, callee saves, and dynamic
stack plans. Call construction joins call plans with ABI argument/result
locations, preserved values, clobbered registers, memory returns, and indirect
callee metadata.

### 4. Move, Spill/Reload, And Parallel-Copy Evidence

The file records multiple movement systems:

- value-location move bundles and ABI bindings
- regalloc move resolution
- spill/reload operations
- out-of-SSA parallel-copy bundles and resolved target moves

These are currently collected into records in one place, which makes the file a
crossroads for allocation, ABI, and SSA-exit evidence.

### 5. Limited Machine Node And MIR Assembly

The file builds a provisional MIR function from selected `codegen` instruction
records. Current node sources are spill/reload, selected return scalar ALU, and
return instructions. Generic MIR operands are converted from `codegen` operands.

### 6. Data And Relocation Records

Module data extraction covers globals, string constants, visibility, initializer
symbols, initializer-element symbols, and relocation-needs collection. This is
separate from function lowering but lives in the same file.

## Fast Paths, Compatibility Paths, And Overfit Pressure

### Core Lowering

- Handoff validation through `abi::validate_prepared_module_handoff`.
- Register conversion through structured `PreparedRegisterPlacement` when
  present.
- Operand, frame, call, move, spill/reload, parallel-copy, global, string, and
  relocation records derived from prepared plans.

### Optional Fast Paths

- `build_return_scalar_alu_machine_nodes` recognizes returned `add`/`sub`
  scalar binary instructions and emits record-only scalar ALU machine nodes
  when locations and storage agree.
- `build_return_machine_nodes` can reuse a scalar ALU result register for the
  return value.
- Immediate return values become immediate operands directly.

### Legacy Compatibility

- Register conversion accepts legacy register-name strings and reparses them
  if structured conversion fails.
- Label/function matching falls back from structured IDs to string labels.
- Occupied-register views can be rebuilt from parsed register references or
  preserved from fallback string lists.
- Storage and value-home paths support immediate, stack, register, symbol, and
  non-register encodings even when not all structured fields are present.

### Overfit Pressure

- The return scalar ALU path is narrow: it only selects `Add` and `Sub`, only
  when the result is returned, and has a fallback path that manually constructs
  a return-ABI scalar instruction. This is behavior evidence, not a rebuild
  design pattern to expand case by case.
- The file mixes record assembly with opportunistic machine-node synthesis. A
  rebuild should avoid adding more named-operation shortcuts here.
- Register-name fallback logic is necessary compatibility evidence, but new
  structured paths should not be forced through strings.
- Function/block source recovery by spelling is fragile and should remain a
  compatibility bridge rather than the primary identity model.

## Rebuild Ownership Boundary

This file should own the final composition of an AArch64 module snapshot from
validated prepared inputs: ordering the record builders, preserving source
evidence, and returning the public `BuildResult`.

It should not own register allocation, ABI classification, prepared-plan
construction, broad name recovery policy, scalar instruction selection, generic
MIR operand semantics, or data-layout policy. In a rebuild, those should move
behind narrower helpers or existing domain modules, with this file reduced to a
thin orchestrator over explicit contracts.
