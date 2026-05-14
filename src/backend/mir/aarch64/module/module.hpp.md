# `src/backend/mir/aarch64/module/module.hpp` Replacement Draft

## Purpose

`module.hpp` is the single non-helper public header for the AArch64 module
replacement. It declares the stable public build entry point, the public module
product, the durable canonical MIR carrier vocabulary, lightweight provenance,
compatibility projections, and target-owned printable surfaces.

This draft is a Stage 3 contract only. It does not describe the legacy header
as the desired implementation, and it does not authorize edits to real `.hpp`,
`.cpp`, build, or test files.

## Public Entry Point

The public entry point remains:

```cpp
[[nodiscard]] BuildResult build(const prepare::PreparedBirModule& prepared);
```

`build` accepts prepared BIR as the authority for the AArch64 module handoff.
It returns a completed module product when the prepared target profile and
AArch64 ABI handoff validate, otherwise it returns `abi::HandoffError`.

```cpp
struct BuildResult {
  std::optional<Module> module;
  std::optional<abi::HandoffError> error;
};
```

`BuildResult` is a transport result only. It must not expose partially lowered
record piles as a fallback product.

## Public Module Product

The public module product keeps the stable module handoff while making the
canonical MIR carrier the durable output.

```cpp
struct Module {
  c4c::TargetProfile target_profile;
  MachineModule mir;
  ModuleDataRecords data;
  CompatibilityProjection compatibility;
};
```

`Module` may retain enough diagnostic provenance to explain where facts came
from, but the replacement contract should not require raw prepared/source
object ownership or `std::string_view` lifetime coupling as semantic
authority. Any retained raw pointers or views belong in compatibility and
diagnostic projection records with explicit lifetime notes.

## Canonical MIR Carrier

The canonical output is hierarchical typed MIR:

```cpp
struct MachineModule {
  std::vector<MachineFunction> functions;
  std::vector<ModuleDataItem> data_items;
  std::vector<RelocationNeed> relocation_needs;
};

struct MachineFunction {
  c4c::FunctionNameId function_name;
  LabelSymbol label;
  std::vector<MachineBlock> blocks;
  Provenance provenance;
};

struct MachineBlock {
  c4c::BlockLabelId block_label;
  LabelSymbol label;
  std::vector<MachineInstruction> instructions;
  std::vector<BlockSuccessor> successors;
  Provenance provenance;
};
```

The exact container spelling can follow the shared MIR library, but the
semantic shape is fixed: module contains functions, functions contain blocks,
and blocks contain ordered AArch64 machine instructions or control nodes.
`FunctionRecord::machine_nodes` and any other flat vector are compatibility
views only.

## Target Instruction And Operand Surfaces

AArch64 owns the printable target surfaces consumed by the shared
`mir_printer`:

```cpp
struct MachineInstruction {
  AArch64Opcode opcode;
  std::vector<MachineOperand> operands;
  InstructionProvenance provenance;

  void render_instruction(TargetRenderSink& sink) const;
};

struct MachineOperand {
  MachineOperandKind kind;
  OperandStorage storage;
  OperandProvenance provenance;

  void render_operand(TargetRenderSink& sink) const;
};

struct MachineRegister {
  RegisterBank bank;
  uint32_t number;
  RegisterWidth width;

  void render_register(TargetRenderSink& sink) const;
};
```

Printable target objects cover instructions, operands, registers, memory
references, labels, symbols, immediates, and relocation-aware forms. Instruction
rendering and operand rendering remain separate surfaces so the shared printer
does not learn AArch64 syntax.

Target-owned render APIs use ordinary C++ names such as
`render_instruction`, `render_operand`, and `render_register`. The replacement
must not introduce a C++ method or public API named `__repr__`.

## Operand Authority Vocabulary

Operand resolution produces a typed location model before instruction, branch,
or call lowering consumes operands:

```cpp
enum class OperandAuthority {
  PreparedValueHome,
  StoragePlan,
  RegallocAssignment,
  FrameSlot,
  SpillSlot,
  AbiLocation,
  Immediate,
  Symbol,
  Label,
  CompatibilityFallback
};

struct OperandStorage {
  OperandAuthority authority;
  std::variant<
      MachineRegister,
      FrameSlotRef,
      SpillSlotRef,
      ImmediateValue,
      MemoryReference,
      LabelSymbol,
      GlobalSymbol> value;
};
```

Downstream lowering components consume `MachineOperand` and `MachineRegister`
values instead of rechecking broad prepared tables or public records. Storage
precedence belongs to operand resolution. Register spelling fallback is
fail-closed compatibility behavior and cannot override structured prepared
facts.

## Provenance Model

Provenance is the `MachineOrigin`-equivalent lightweight metadata attached to
module, function, block, instruction, operand, and compatibility records:

```cpp
struct Provenance {
  std::optional<c4c::FunctionNameId> function_name;
  std::optional<c4c::BlockLabelId> block_label;
  std::optional<c4c::ValueNameId> value_name;
  std::optional<size_t> prepared_index;
  std::optional<SourceLocationRef> source_location;
  ProvenanceReason reason;
};
```

Allowed provenance data includes stable IDs, indices, labels, durable source
locations, and reason tags that identify the prepared authority used. The
replacement model must not make raw prepared/source ownership, cached display
strings, or source spelling recovery the authority for lowering.

## Module Data Records

Module data remains a public product, but it is not a second lowering path:

```cpp
struct ModuleDataRecords {
  std::vector<GlobalDataRecord> globals;
  std::vector<StringDataRecord> strings;
  std::vector<DataRelocationNeedRecord> relocation_needs;
};
```

Data records come from prepared module data and relocation requirements. The
public assembly bridge passes canonical module data to the shared printer
through target-owned hooks; AArch64 does not own common section traversal or
file emission structure.

## Compatibility Projection Boundary

Compatibility projections exist for migration and diagnostics after canonical
MIR lowering completes:

```cpp
struct CompatibilityProjection {
  std::vector<FunctionRecord> functions;
  std::vector<DiagnosticRecord> diagnostics;
};

struct FunctionRecord {
  c4c::FunctionNameId function_name;
  MachineFunctionView mir;
  std::vector<codegen::InstructionRecord> machine_nodes;
  FunctionInspectionRecords inspection;
};
```

Compatibility may include `FunctionRecord::machine_nodes`, broad inspection
records, source/prepared pointer views, label views, legacy register-string
diagnostics, frame/call/move/spill/parallel-copy inspection records, and other
legacy shapes needed by existing clients. These records are derived from
`MachineModule`, typed operands, prepared ABI facts, and lightweight
provenance after lowering.

Compatibility projections must not:

- drive instruction, branch, call, or operand lowering
- choose among optional broad fields to satisfy a testcase
- become the primary print or validation carrier
- make flat `machine_nodes` equivalent to canonical MIR blocks
- use cached display strings or spelling recovery as semantic authority

## Shared Printer Contract

The shared `mir_printer` owns traversal and common emission mechanics:
function order, block order, label placement, sections, data traversal,
spacing mechanics, and `.s` file structure. AArch64 owns the concrete text for
target instructions, operands, registers, memory references, labels, symbols,
immediates, opcodes, and target data hooks.

The header should expose only the target-owned printable concepts required by
that printer bridge. It must not make the printer call into compatibility
records or flat machine-node vectors.

## Owned Inputs

- `prepare::PreparedBirModule`
- prepared target profile and AArch64 handoff records
- prepared control-flow functions and blocks
- prepared frame, storage, value home, regalloc, spill/reload,
  parallel-copy, call-plan, global, string, and relocation facts
- shared ID namespaces and source-location references when already durable
- shared MIR container and printer concepts

## Owned Outputs

- `BuildResult`
- `Module`
- `MachineModule`, `MachineFunction`, `MachineBlock`, and
  `MachineInstruction` or equivalent typed MIR concepts
- AArch64 `MachineOperand`, `MachineRegister`, memory, label, symbol,
  immediate, and opcode printable surfaces
- lightweight `Provenance`
- `ModuleDataRecords`
- `CompatibilityProjection`

## Indirect Queries

The public header may name ABI, prepared BIR, shared MIR, codegen record, and
ID types needed to express the public contract. It should not require clients
to understand private lowering contexts, helper state, or component internals
from module dispatch, traversal, operand resolution, instruction lowering,
branch/control lowering, call lowering, public assembly bridging, or
compatibility projection.

## Forbidden Knowledge

The header must not encode:

- operation-specific lowering algorithms
- register-allocation policy or frame planning policy
- call ABI planning internals beyond public projection shapes
- branch fusion implementation details
- spill/reload or parallel-copy scheduling algorithms
- printer traversal or `.s` file structure
- broad optional public records as semantic input
- cached display strings, source spelling recovery, or flat vectors as the
  lowering authority

## Header Policy

`module.hpp` remains the only non-helper public header draft for this
directory. Stage 3 must not add component-level public headers for the
implementation seams.

`helper.hpp` is the only allowed exception. It may be drafted only after a
later packet explicitly proves all of these conditions:

- the declarations are private to this replacement directory
- multiple implementation drafts must share the declarations
- placing the declarations in one `.cpp.md` would create hidden two-way
  coupling or duplicate incompatible private contracts
- the helper does not become a second public index and does not expose
  lowering internals to external clients

No `helper.hpp.md` is mandatory in Stage 3.
