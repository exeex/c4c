# `src/backend/mir/aarch64/module/branch_control_lowering.cpp` Replacement Draft

## Purpose

`branch_control_lowering.cpp` owns AArch64 lowering for prepared terminators,
branch/control instructions, return control flow, and canonical block
successor metadata. It consumes prepared control-flow authority and typed
operands from `operand_resolution.cpp`; it does not recover branch semantics
from public branch records, source spellings, cached display strings, or flat
compatibility vectors.

This draft is a Stage 3 replacement draft artifact only. It does not describe
the compiled legacy implementation, and it does not authorize edits to real
source, build, or test files.

The replacement route is:

`prepared terminator and successor facts -> typed condition/label operands ->
AArch64 branch/control MachineInstruction nodes -> MachineBlock successors`.

## Classification

Core control-flow lowering with an optional branch-fusion fast path.

This file is not module dispatch, function traversal, operand authority,
ordinary instruction lowering, call lowering, public assembly bridging, or
compatibility projection.

## Lowering Contract

Branch/control lowering is reached from function traversal after non-control
operations and call-owned sequences for the current prepared block have been
lowered:

```cpp
void lower_prepared_terminator(
    const PreparedTerminatorRef& terminator,
    FunctionLoweringContext& context,
    MachineBlock& block,
    ModuleLoweringDiagnostics& diagnostics);
```

The spelling above is illustrative. The required contract is that one
prepared block terminator lowers into ordered target control
`MachineInstruction` nodes and complete `BlockSuccessor` metadata for the
current canonical `MachineBlock`. Missing or contradictory prepared
terminator, target-label, compare, condition, or successor authority is a
lowering error.

## Prepared Terminators

Prepared terminators are the only semantic authority for block exits. This
seam may read:

- prepared terminator kind
- prepared successor identities and edge ordering
- prepared branch labels and block label IDs
- prepared condition and compare metadata
- prepared return-control and return-value plan references
- durable source-location references already linked to prepared identity for
  optional provenance

It must not derive terminator kind, edge labels, fallthrough decisions, or
return behavior from source text, public branch records, rendered assembly, or
compatibility `machine_nodes`.

## Conditional Branches

Conditional branch lowering consumes prepared compare/condition facts and
typed operands for the compared values, flags, immediates, or condition codes.
It emits target branch/control `MachineInstruction` nodes such as compare,
test, conditional branch, or equivalent AArch64 forms.

Conditional lowering must also populate successor metadata for both taken and
not-taken edges using prepared block successor identities. Fallthrough is a
control-flow fact from prepared successor order and block layout; it is not
recovered by comparing source labels or rendered text.

## Unconditional Branches

Unconditional branch lowering consumes a prepared single-successor terminator
and a typed target label operand from operand resolution. It emits the target
branch instruction when control must jump explicitly and records successor
metadata for the edge.

If the canonical block layout permits fallthrough without an emitted branch,
the decision must be based on prepared successor identity, canonical block
order, and target branch constraints. The successor metadata remains explicit
even when no branch instruction is emitted.

## Compare And Condition Handling

Compare and condition handling is owned here because it determines branch
control semantics. This seam may choose whether to:

- emit an explicit compare or test instruction
- consume a prepared flag-producing instruction result
- select a condition-code form
- invert a condition to satisfy target branch layout constraints
- materialize condition operands through typed operand resolution

The choice is governed by prepared comparison facts, target constraints, and
canonical block layout. It must not inspect public branch records or parse
condition text from cached display strings.

## Valid Branch Fusion

Branch fusion is an optional fast path, not a compatibility requirement. It is
valid only when prepared comparison facts, target opcode constraints, operand
types, block layout, and successor metadata prove that a compare-producing
operation can be fused with the following branch without changing semantics.

Fusion may eliminate or combine a compare/control sequence into a target
branch form. It must preserve instruction provenance for both prepared
authorities and must produce the same canonical successor metadata as the
unfused path.

Fusion is invalid when it depends on source spelling, public branch records,
cached display strings, broad optional inspection records, or a named testcase
shape rather than prepared semantic facts.

## Return Control Flow

Return control lowering owns the final target return instruction or return
control node and block sealing for prepared return terminators.

Return-value materialization belongs to `instruction_lowering.cpp` or
call-lowering when governed by a call plan. This file consumes the prepared
return-control fact and verifies that required return-value materialization or
ABI result locations have been handled through typed facts before emitting the
return control instruction.

Immediate returns, scalar ALU result reuse, and ABI return-register copies are
not special branch testcases. They are prepared return-value materialization
facts consumed before this seam emits the final return control node.

## Successor Metadata

Every lowered terminator produces canonical successor metadata on the current
`MachineBlock`:

```cpp
BlockSuccessor successor {
  .target = prepared_successor_label,
  .kind = edge_kind,
  .condition = optional_condition_metadata,
  .provenance = provenance_for_successor(terminator, edge_index),
};
```

The exact C++ spelling can follow the shared MIR carrier. The semantic
requirement is explicit: `MachineBlock::successors` is derived from prepared
control-flow facts and kept consistent with emitted branch/control
`MachineInstruction` nodes. Public branch records are later projections, not
the source of successor truth.

## Emitted MachineInstruction Nodes

Branch/control instructions are canonical MIR target nodes in the current
`MachineBlock`:

```cpp
MachineInstruction instruction {
  .opcode = selected_branch_or_control_opcode,
  .operands = resolved_control_operands,
  .provenance = provenance_for_terminator(terminator, reason),
};
```

Emitted nodes include conditional branches, unconditional branches, compares
when owned by control lowering, tests, return control instructions, and other
target control forms required by prepared terminators. The shared
`mir_printer` later traverses canonical blocks; this file does not own label
printing or assembly file layout.

## Operand And Label Consumption

Branch/control lowering consumes typed operands through narrow queries:

- compare source operands
- condition-code or flag operands
- immediate compare operands
- branch target labels
- fallthrough and explicit successor labels
- return-control operands when target forms require them
- return-value location checks through prepared ABI facts

It may reject a typed operand that violates target branch constraints. It must
not choose among value homes, storage plans, regalloc assignments, ABI
locations, or public records itself; operand precedence belongs to operand
resolution.

## Owned Inputs

- prepared terminators
- prepared block successor identities and edge ordering
- prepared branch labels and block label IDs
- prepared compare and condition metadata
- prepared return-control facts and return-value plan references
- current `FunctionLoweringContext`
- current `MachineBlock` insertion point
- typed `MachineOperand`, `MachineRegister`, and `LabelSymbol` values from
  operand resolution
- target profile branch, compare, condition-code, and return opcode
  constraints
- durable source-location references already linked to prepared identity for
  optional provenance

## Owned Outputs

- ordered target branch/control `MachineInstruction` nodes appended to
  canonical `MachineBlock` instruction storage
- complete `MachineBlock::successors` metadata
- branch/control instruction and edge provenance
- diagnostics for unsupported, ambiguous, missing, or contradictory
  terminator authority
- optional compatibility branch facts derived later from canonical typed
  control lowering

## Indirect Queries

Branch/control lowering may indirectly query:

- operand resolution for typed compare operands, condition operands, branch
  labels, return ABI locations, and target registers
- target profile metadata for branch, compare, condition-code, and return
  instruction forms
- function-context block identity maps and canonical block order
- prepared comparison, branch-fusion, and successor metadata through the
  narrow function context
- instruction-lowering-produced typed state only through prepared value
  identities and canonical block contents, not compatibility records

It must not query call-lowering ABI sequence internals, public assembly bridge
output, compatibility projection records, public branch records, source BIR
terminator spellings, or `FunctionRecord::machine_nodes` while deciding
control semantics.

## Forbidden Knowledge

`branch_control_lowering.cpp` must not know:

- module-level public record assembly
- function traversal order beyond current block identity, canonical successor
  checks, and layout facts provided by the context
- ordinary scalar ALU, memory, move, spill/reload, or parallel-copy opcode
  selection details except for valid branch-fusion facts
- call ABI planning internals and call-site sequence ownership
- public assembly file structure, section traversal, punctuation, or spacing
- compatibility projection layout as semantic branch authority
- broad public branch or inspection records as semantic inputs
- source label or terminator spellings as branch authority
- cached display strings as branch, condition, operand, label, or return
  authority
- flat `FunctionRecord::machine_nodes` vectors as canonical control output
- target render APIs named `__repr__`

## Downstream Use

Function traversal seals the current block after branch/control lowering
confirms that emitted control nodes and successor metadata agree.
Compatibility projection may derive public branch records after canonical MIR
exists. The public assembly bridge and shared printer consume canonical
`MachineBlock` instructions and successors; they must not ask this seam to
re-walk source control flow for printing.

## Replacement Rejection Signals

Reject an implementation of this draft if it:

- treats public branch records, public inspection records, or
  `FunctionRecord::machine_nodes` as branch/control authority
- recovers target labels, condition text, successor edges, or return behavior
  from source spellings or cached display strings
- implements branch fusion because a named testcase shape matches rather than
  because prepared comparison facts and target constraints prove it valid
- omits canonical `MachineBlock::successors` metadata when an emitted branch
  appears to encode the edge
- lowers into a flat vector instead of canonical branch/control
  `MachineInstruction` nodes in `MachineBlock`
- chooses among value homes, storage plans, regalloc assignments, labels, or
  ABI locations outside the typed operand-resolution seam
- mixes call ABI sequence ownership or ordinary instruction family lowering
  into terminator lowering
