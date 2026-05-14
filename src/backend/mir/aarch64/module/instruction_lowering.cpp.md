# `src/backend/mir/aarch64/module/instruction_lowering.cpp` Replacement Draft

## Purpose

`instruction_lowering.cpp` owns AArch64 lowering for prepared non-call,
non-terminator operations into canonical MIR `MachineInstruction` nodes. It
consumes typed prepared operation semantics and typed operands from
`operand_resolution.cpp`; it does not rediscover value locations from broad
public records, cached display strings, source spellings, or flat
compatibility vectors.

This draft is a Stage 3 replacement draft artifact only. It does not describe
the compiled legacy implementation, and it does not authorize edits to real
source, build, or test files.

The replacement route is:

`prepared operation semantics -> typed MachineOperand values -> AArch64Opcode
selection -> MachineInstruction nodes in MachineBlock`.

## Classification

Core instruction lowering.

This file owns semantic instruction families for ordinary target instructions.
It is not module dispatch, function traversal, operand authority, branch or
call lowering, public assembly bridging, or compatibility projection.

## Lowering Contract

Instruction lowering is reached from function traversal after the traversal
layer has created the current `MachineFunction` and `MachineBlock` insertion
point:

```cpp
void lower_prepared_operation(
    const PreparedOperationRef& operation,
    FunctionLoweringContext& context,
    MachineBlock& block,
    ModuleLoweringDiagnostics& diagnostics);
```

The spelling above is illustrative. The required contract is that one
prepared non-call, non-terminator operation lowers into zero or more ordered
target `MachineInstruction` nodes appended to the current block. Every emitted
node carries typed opcode, typed operands, and lightweight provenance. Missing
or contradictory prepared authority is a lowering error, not a request to
consult compatibility projections.

## Semantic Families

Instruction lowering is organized by semantic operation family, not by named
testcase or by legacy record shape. The initial required families are:

- scalar ALU operations
- memory operations
- moves and materializations
- spill and reload operations
- parallel-copy steps
- return-value materialization

Each family maps prepared semantics to target opcode constraints, asks operand
resolution for typed source/destination operands, validates target register
classes and immediate encoding constraints, and emits canonical
`MachineInstruction` nodes.

## Scalar ALU

Scalar ALU lowering covers prepared integer arithmetic, logical operations,
shifts, and related value-producing operations when prepared BIR semantics and
target constraints are available.

The scalar ALU path may read:

- prepared operation kind and result value identity
- prepared operand value identities and literal operands
- signedness, width, and condition-relevant flags when supplied by prepared
  semantics
- typed source, destination, and immediate operands from operand resolution
- target opcode constraints for AArch64 register and immediate forms

It emits target `MachineInstruction` nodes such as add, sub, logical, shift,
or equivalent AArch64 opcode-family forms. Returned `add` and `sub` are not a
special testcase path; they are normal scalar ALU instructions whose result can
later be consumed by return-value materialization or branch/control lowering.

## Memory Operations

Memory lowering covers prepared loads, stores, address materializations, and
memory-reference operations that are not owned by call lowering.

The memory path asks operand resolution for typed memory references,
frame/spill/global/label address forms, source or destination registers, and
immediates. It chooses load/store opcode families from prepared access size,
alignment, signedness, volatility/order metadata when prepared, and target
register class constraints.

Memory lowering must not parse address text or infer frame slots from rendered
assembly. Address authority belongs to operand resolution; opcode and
instruction-sequence authority belongs here.

## Moves And Materializations

Move lowering covers prepared value-location moves, constant materializations,
register-to-register moves, frame/spill address moves, symbol or label
materializations, and target-register copies that are not call-ABI argument or
result moves.

Moves consume typed source and destination operands. They may emit one
instruction or a target-specific sequence when a value cannot be represented
by a single AArch64 move form. Multi-instruction materialization remains one
semantic family: the sequence is derived from prepared value and operand
authority, not from compatibility display text.

## Spill And Reload

Spill/reload lowering consumes prepared spill and reload authorities plus
typed operands for the spilled value, spill slot, reload target, and memory
reference.

Spill lowering emits store-family `MachineInstruction` nodes from a typed
register operand to a typed spill or frame memory reference. Reload lowering
emits load-family nodes from a typed spill or frame memory reference to a
typed register operand. Slot identity, stack offset, and register authority
come from operand resolution and the function context; this file only chooses
the target load/store instruction family and appends nodes.

## Parallel Copy

Parallel-copy lowering consumes prepared parallel-copy bundles that have
already been scheduled or decomposed into move steps by prepared authority.
Each move step asks operand resolution for typed source and destination
operands and emits the required copy, exchange, scratch-register move, or
memory-assisted sequence.

This draft does not authorize instruction lowering to invent a new scheduler
from broad public move records. If prepared parallel-copy authority is
insufficient to produce an acyclic or scratch-backed sequence, lowering fails
closed with diagnostics.

## Return-Value Materialization

Return-value materialization prepares return values for branch/control return
lowering without owning the return terminator itself.

This family consumes prepared return-value plans, ABI return locations, typed
source operands, and typed target ABI registers or memory-return locations. It
emits only the value movement or materialization instructions needed before
the final return control instruction. The actual return branch/control node,
block sealing, and successor metadata belong to
`branch_control_lowering.cpp`.

Immediate returns, scalar ALU result reuse, and ABI result-register copies are
semantic return-value materialization cases. They must not be implemented as
named returned-expression shortcuts or by reading cached display strings.

## Emitted MachineInstruction Nodes

Every emitted instruction is a canonical MIR target node:

```cpp
MachineInstruction instruction {
  .opcode = selected_opcode,
  .operands = resolved_machine_operands,
  .provenance = provenance_for_operation(operation, family_reason),
};
```

The exact C++ spelling can follow the shared MIR carrier, but the semantic
output is fixed: ordered target `MachineInstruction` nodes inside the current
`MachineBlock`. `FunctionRecord::machine_nodes` may later receive a derived
compatibility view, but it is never the instruction-lowering product.

## Operand Consumption

Instruction lowering consumes typed operands through narrow queries:

- source and destination value operands
- explicit target-register operands
- immediate operands and immediate encoding classes
- memory references, frame slots, and spill slots
- symbol and label materialization operands
- return ABI location operands
- parallel-copy step operands

It may reject a typed operand that violates the selected instruction family or
target opcode constraints. It must not choose among prepared value homes,
storage plans, regalloc assignments, frame slots, spill slots, or public
inspection records itself; that storage precedence belongs to operand
resolution.

## Owned Inputs

- prepared non-call, non-terminator operation semantics
- current `FunctionLoweringContext`
- current `MachineBlock` insertion point
- typed `MachineOperand` and `MachineRegister` values from operand resolution
- prepared scalar ALU, memory, move, spill/reload, parallel-copy, and
  return-value materialization facts
- target profile opcode, register-class, width, and immediate constraints
- durable source-location references already linked to prepared identity for
  optional provenance

## Owned Outputs

- ordered target `MachineInstruction` nodes appended to canonical
  `MachineBlock` instruction storage
- instruction provenance for prepared operation and semantic-family reasons
- diagnostics for unsupported, ambiguous, missing, or contradictory
  instruction authority
- private lowering facts needed by later branch/control or compatibility
  projection, when derived from canonical typed instruction lowering

## Indirect Queries

Instruction lowering may indirectly query:

- operand resolution for typed operands, registers, immediates, memory forms,
  symbols, labels, spill slots, frame slots, and return ABI locations
- target profile metadata for opcode forms, register classes, widths, and
  immediate encodings
- function-context identity maps for provenance and diagnostics
- prepared parallel-copy and spill/reload authority through the narrow
  function context

It must not query branch/control lowering internals, call-lowering ABI
sequence internals, public assembly bridge output, compatibility projection
records, or `FunctionRecord::machine_nodes` while deciding instruction
semantics.

## Forbidden Knowledge

`instruction_lowering.cpp` must not know:

- module-level public record assembly
- function or block traversal order beyond the active insertion point
- prepared terminator edge ownership or block sealing rules
- call ABI planning internals and call-site sequence ownership
- public assembly file structure, section traversal, punctuation, or spacing
- compatibility projection layout as semantic instruction authority
- broad public inspection records as semantic inputs
- `FunctionRecord::machine_nodes` or flat `machine_nodes` vectors as
  canonical output
- cached display strings as instruction, operand, register, label, immediate,
  symbol, or memory authority
- source spelling recovery as the primary identity model
- target render APIs named `__repr__`

## Downstream Use

Function traversal appends the emitted nodes to the current block. Branch and
control lowering may consume the resulting typed value locations only through
prepared value identities, operand resolution, and canonical block state; it
must not inspect compatibility instruction records to decide return or branch
semantics. Compatibility projection may derive public instruction records
after canonical MIR exists.

## Replacement Rejection Signals

Reject an implementation of this draft if it:

- adds named-testcase handling for returned `add`, returned `sub`, or another
  narrow source expression instead of semantic scalar ALU and return-value
  materialization families
- treats cached display strings as opcode, operand, register, label, symbol,
  immediate, or memory authority
- asks compatibility projection, public inspection records, or
  `FunctionRecord::machine_nodes` to choose instruction semantics
- lowers into a flat vector instead of canonical `MachineInstruction` nodes in
  `MachineBlock`
- chooses among value homes, storage plans, regalloc assignments, spill slots,
  frame slots, or ABI locations outside the typed operand-resolution seam
- recovers missing structured identity from source spelling instead of
  reporting a handoff or lowering error
- mixes call ABI sequence ownership or terminator edge ownership into ordinary
  instruction lowering
