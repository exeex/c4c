# `src/backend/mir/aarch64/module/operand_resolution.cpp` Replacement Draft

## Purpose

`operand_resolution.cpp` owns the AArch64 typed operand authority seam and the
single typed prepared value-location authority model. It converts validated
`prepare::PreparedBirModule` facts into typed `MachineOperand` and
`MachineRegister` values before instruction, branch/control, or call lowering
chooses target opcodes.

This draft is a Stage 3 replacement draft artifact only. It does not describe
the compiled legacy implementation, and it does not authorize edits to real
source, build, or test files.

The replacement route is:

`prepared value/storage authority -> OperandAuthority precedence ->
MachineRegister / MachineOperand -> semantic lowerers`.

Instruction, branch, and call lowering consume typed operands from this seam.
They do not choose among broad optional public records, cached display strings,
or flat compatibility vectors.

## Classification

Core operand authority.

This file is not module dispatch, function traversal, instruction selection,
call ABI expansion, public assembly bridging, or compatibility projection. It
is the typed value-location and target-operand service shared by those
lowerers.

## Operand Resolution Contract

Operand resolution is reached through narrow per-function context queries
created by function traversal:

```cpp
MachineOperand resolve_value_operand(
    ValueLocationQuery query,
    const FunctionLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics);

MachineRegister resolve_target_register(
    TargetRegisterQuery query,
    const FunctionLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics);
```

The spelling above is illustrative. The required contract is that callers ask
for an operand or register for one concrete prepared value, storage location,
immediate, symbol, label, memory reference, ABI location, or target register
fact. The result is typed and provenance-bearing. Missing or contradictory
authority is reported as a lowering error; it is not recovered from source
spellings.

## Authority Sources

This seam may read the following prepared authorities through the typed
function context:

- `prepare::PreparedBirModule`
- prepared value homes
- storage plans
- regalloc assignments
- frame slots
- spill slots
- ABI locations supplied by call plans or return-value plans
- immediates
- symbols
- labels
- target registers
- relocation-aware symbol or label references
- durable source locations already linked to prepared identity for provenance

The seam converts those facts into `OperandAuthority`, `OperandStorage`,
`MachineOperand`, and `MachineRegister` values from the Step 1 header
vocabulary.

## Storage Precedence

Storage precedence is centralized here so downstream lowerers do not each
interpret a broad pile of optional records. For a value query, resolution uses
the first applicable structured authority in this order:

1. Explicit operation or ABI target-register operand required by a prepared
   instruction, branch, call, or return plan.
2. Regalloc assignment for the prepared value at the queried program point.
3. Storage-plan entry for the value at the queried program point.
4. Prepared value home when no narrower point-specific storage exists.
5. Frame slot or spill slot when the value is explicitly materialized through
   a prepared frame, spill, reload, or stack-memory authority.
6. Immediate, symbol, label, or memory-reference authority when the operand is
   not a value location but a prepared literal, relocation, block target, global
   symbol, or address form.

The concrete implementation can split this order by query kind, but it must
preserve the semantic rule: the most specific prepared authority wins, and
compatibility records never arbitrate storage.

If two structured authorities conflict at the same specificity, operand
resolution fails closed with diagnostics. It must not silently prefer whichever
optional public field happens to be populated.

## Value Homes

Prepared value homes describe durable homes for values across the function.
Operand resolution maps them into:

- `MachineRegister` operands for register homes
- `MemoryReference` operands for frame-relative homes
- `FrameSlotRef` or `SpillSlotRef` operands when a later lowerer needs a slot
  identity instead of a rendered address
- provenance reason `OperandAuthority::PreparedValueHome`

Value homes do not override a narrower regalloc assignment, storage-plan entry,
or explicit prepared target-register requirement for the current operation.

## Storage Plans

Storage plans describe point-sensitive storage transitions such as move,
parallel-copy, spill, reload, and materialization sites. Operand resolution maps
storage-plan facts into typed operands with provenance reason
`OperandAuthority::StoragePlan`.

Storage plans may name source and destination locations. Each side is resolved
independently through the same typed location machinery so move and
parallel-copy lowering receives `MachineOperand` values rather than broad
storage records.

## Regalloc Assignments

Regalloc assignments are the preferred source for values currently resident in
physical or virtual target registers. Operand resolution validates the register
bank, number, width, and target profile before producing `MachineRegister`.

Register assignment results carry provenance reason
`OperandAuthority::RegallocAssignment`. If a regalloc assignment names a
register that is inconsistent with the target profile, expected register
class, or operation width, resolution reports a lowering error instead of
falling back to legacy register strings.

## Frame And Spill Slots

Frame slots and spill slots are structured stack authorities:

- frame slots map to `FrameSlotRef` or concrete `MemoryReference` operands
- spill slots map to `SpillSlotRef` or concrete `MemoryReference` operands
- frame-relative addresses preserve base register, offset, size, alignment, and
  access kind when prepared facts provide them

Frame and spill operands carry `OperandAuthority::FrameSlot` or
`OperandAuthority::SpillSlot` provenance. Rendering a stack address is a later
target-owned operand-printing concern; resolving that the value lives in a
frame or spill slot belongs here.

## Immediates

Prepared immediates become typed `MachineOperand` values with
`OperandAuthority::Immediate`. The resolved operand preserves width,
signedness, relocation needs, and instruction-family constraints that the
prepared authority already provides.

If a target opcode needs an immediate encoding class, instruction lowering may
ask operand resolution for that class through a narrow query. Instruction
lowering must not parse numeric text from cached display strings.

## Symbols And Labels

Prepared symbols and labels become `GlobalSymbol`, `LabelSymbol`, or
relocation-aware operand forms with `OperandAuthority::Symbol` or
`OperandAuthority::Label`.

Operand resolution may validate that a branch target label belongs to the
current prepared function, that a global symbol exists in prepared module data,
and that relocation metadata is available when required. It must not recover
block labels, function names, or globals from rendered assembly strings.

## Memory References

Memory references are typed operands assembled from prepared base, offset,
index, frame, spill, global, or label facts. They preserve the address-space
and access-size facts required by AArch64 operand rendering.

Memory-reference resolution can produce a `MachineOperand` whose storage holds
`MemoryReference`, or it can produce an intermediate `FrameSlotRef` or
`SpillSlotRef` when the caller is still deciding whether to emit an address
calculation, load, store, spill, or reload sequence.

The seam owns address authority. It does not choose the load/store opcode
family; that belongs to instruction lowering or call lowering depending on the
semantic operation.

## Target Registers

Prepared target-register facts become `MachineRegister` values after target
profile validation. Resolution checks:

- register bank
- register number
- width
- ABI role when supplied by call or return plans
- operation or storage register-class constraints

The output is a typed `MachineRegister`; render spelling such as `x0`, `w0`,
`sp`, or `lr` is produced later by the target render surface. Register spelling
fallback cannot create or replace a structured register.

## Fail-Closed Legacy Register-String Fallback

Legacy register-string fallback is compatibility diagnostics only. It exists
to explain old records or migration mismatches after structured authority has
failed or when compatibility projection needs to show legacy text.

Fallback rules:

- it is fail-closed and cannot synthesize semantic `MachineRegister` authority
- it cannot override prepared value homes, storage plans, regalloc assignments,
  frame slots, spill slots, immediates, symbols, labels, memory references, or
  target-register facts
- it may emit diagnostics that name the legacy string and the missing
  structured authority
- it belongs on a compatibility or diagnostic path, not on the main lowering
  path

## Owned Inputs

- validated `prepare::PreparedBirModule`
- per-function lowering context from function traversal
- prepared value homes
- prepared storage plans
- prepared regalloc assignments
- prepared frame slots and stack-layout facts
- prepared spill slots and spill/reload facts
- prepared ABI locations from call and return plans
- prepared immediates, symbols, labels, and relocation facts
- prepared memory-reference facts
- prepared target registers and target profile register metadata
- durable source-location references for optional operand provenance

## Owned Outputs

- typed `MachineOperand` values
- typed `MachineRegister` values
- `OperandStorage` values with `OperandAuthority`
- `MemoryReference`, `FrameSlotRef`, `SpillSlotRef`, `ImmediateValue`,
  `LabelSymbol`, and `GlobalSymbol` operands or equivalent carrier forms
- operand and register provenance
- diagnostics for missing, ambiguous, or contradictory operand authority

## Indirect Queries

Operand resolution may indirectly query:

- target profile register metadata
- prepared function/block/value identity maps from the function context
- module data and relocation indexes for prepared symbols
- frame-layout metadata for frame and spill address formation
- call-plan ABI location records when resolving call-adjacent operands

It must not query instruction lowering, branch/control lowering, call-lowering
sequence internals, public assembly bridging, compatibility projection, or
`FunctionRecord::machine_nodes` while deciding operand authority.

## Forbidden Knowledge

`operand_resolution.cpp` must not know:

- scalar ALU, branch, return, or call instruction-selection algorithms
- public assembly file structure, section traversal, punctuation, or spacing
- compatibility projection layout except for diagnostic fallback reporting
- broad public inspection records as semantic inputs
- `FunctionRecord::machine_nodes` semantic authority
- cached display string authority for operands, registers, labels, symbols,
  immediates, or memory references
- source spelling recovery as the primary identity model
- raw prepared/source object ownership as the provenance model
- target render APIs named `__repr__`

## Downstream Use

Instruction lowering, branch/control lowering, and call lowering consume this
seam by requesting typed operands for already-classified semantic needs:

- instruction lowering asks for source, destination, memory, immediate, spill,
  reload, move, and parallel-copy operands
- branch/control lowering asks for compare operands, condition operands,
  branch labels, return-value locations, and successor labels
- call lowering asks for ABI argument/result locations, indirect callee
  operands, preserved-value operands, clobber registers, and memory-return
  locations

Those lowerers may reject an operand that does not satisfy their semantic
operation, but they do not choose among prepared storage authorities
themselves.

## Replacement Rejection Signals

Reject an implementation of this draft if it:

- asks instruction, branch, or call lowering to choose among broad optional
  public records for the same value
- treats `FunctionRecord::machine_nodes` or another flat `machine_nodes`
  vector as semantic authority
- treats cached display strings as register, operand, symbol, label,
  immediate, or memory-reference authority
- recovers missing structured identity from source spelling instead of
  reporting a handoff or lowering error
- uses legacy register-string fallback as a semantic register source
- lets compatibility projection decide value storage precedence
- stores only rendered operand text instead of typed `MachineOperand`,
  `MachineRegister`, and `OperandAuthority` facts
- introduces target render APIs named `__repr__`
