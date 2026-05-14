# `src/backend/mir/aarch64/module/call_lowering.cpp` Replacement Draft

## Purpose

`call_lowering.cpp` owns AArch64 lowering for prepared call plans into
canonical MIR `MachineInstruction` sequences. It consumes call-site ABI facts
and typed operands from `operand_resolution.cpp`; it does not rediscover call
semantics from broad public call records, compatibility projections, cached
display strings, source spellings, or flat `FunctionRecord::machine_nodes`.

This draft is a Stage 3 replacement draft artifact only. It does not describe
the compiled legacy implementation, and it does not authorize edits to real
source, build, or test files.

The replacement route is:

`prepared call plan -> typed ABI operands and locations -> call-adjacent
MachineInstruction sequence -> canonical MachineBlock`.

## Classification

Core ABI call lowering.

This file owns prepared call-plan sequencing and the ABI-owned moves that are
adjacent to a call. It is not module dispatch, function traversal, operand
authority, ordinary instruction lowering, branch/control lowering, public
assembly bridging, or compatibility projection.

## Lowering Contract

Call lowering is reached from function traversal when a prepared operation is
governed by a prepared call plan:

```cpp
void lower_prepared_call(
    const PreparedCallPlanRef& call_plan,
    FunctionLoweringContext& context,
    MachineBlock& block,
    ModuleLoweringDiagnostics& diagnostics);
```

The spelling above is illustrative. The required contract is that one prepared
call plan lowers into an ordered target call sequence appended to the current
canonical `MachineBlock`. The sequence includes ABI argument setup, memory
return setup, direct or indirect call emission, result capture, and
call-adjacent preservation or clobber handling when those steps are governed
by the call plan. Missing or contradictory call-plan, ABI-location, callee, or
operand authority is a lowering error.

## Prepared Call Plans

Prepared call plans are the semantic authority for calls. This seam may read:

- prepared callee identity and call kind
- prepared argument and result location plans
- prepared call-site ABI bindings
- prepared preserved-value and clobber facts
- prepared indirect-callee metadata
- prepared memory-return facts
- prepared call-adjacent move requirements
- durable source-location references already linked to prepared identity for
  optional provenance

It must not derive call kind, callee identity, argument placement, result
placement, clobbers, or memory-return behavior from public call records,
source expressions, rendered assembly, compatibility records, or flat
`machine_nodes`.

## Argument Locations

Argument lowering consumes the prepared argument-location plan and asks
operand resolution for typed source operands and ABI destination locations.

For each argument, call lowering:

1. resolves the argument source value, immediate, symbol, address, or aggregate
   fragment through typed operand queries
2. resolves the ABI destination as a typed register, stack outgoing area, or
   memory reference
3. emits the required call-adjacent move, store, materialization, or address
   setup in prepared ABI order
4. records provenance tying the emitted node to the call plan and argument
   index

Storage precedence remains in operand resolution. This file may reject an
operand that cannot satisfy the ABI location, but it must not choose among
value homes, storage plans, regalloc assignments, frame slots, spill slots, or
public records itself.

## Result Locations

Result lowering consumes prepared result-location and ABI binding facts. It
handles:

- scalar and pointer results returned in ABI registers
- aggregate or wide results split across multiple ABI locations
- memory-return destinations supplied by the caller
- void calls whose result plan carries no value-producing location
- call results that must be copied from ABI locations into prepared value
  homes, regalloc assignments, storage-plan destinations, frame slots, or
  spill slots

The move or materialization from ABI result locations into the prepared value
location is call-adjacent when the call plan owns it. Ordinary later moves
remain in `instruction_lowering.cpp`; the ownership boundary is the prepared
call plan, not the opcode mnemonic.

## Call-Adjacent Moves

Call-adjacent moves are the setup and teardown instructions whose correctness
depends on the call-site ABI plan. They include:

- argument register copies
- outgoing stack argument stores
- address materialization for indirect callees and memory-return pointers
- result-register copies immediately after the call
- save/restore or move-through-scratch steps explicitly required by the call
  plan
- ABI-mandated stack-pointer adjustments when represented as call-site facts

These moves lower to ordinary canonical `MachineInstruction` nodes, but their
sequence ownership is here because their ordering is tied to the call. General
value-location moves, spill/reload operations, and parallel-copy bundles that
are not governed by a call plan stay in ordinary instruction lowering.

## Preserved Values

Prepared preserved-value facts describe values that must survive the call.
Call lowering resolves each preserved value and the required temporary,
callee-save, stack, or scratch location through operand resolution, then emits
the prepared save, restore, or protection sequence around the call.

Preservation is not inferred from public inspection records or by scanning a
flat machine-node list. If the prepared call plan does not prove how a value
is preserved, lowering reports a diagnostic instead of guessing from register
names or source spellings.

## Clobbers

Clobber facts are call-site ABI facts attached to the canonical call
instruction and to any required surrounding sequence. Call lowering validates
clobbered registers and register classes through the target profile and
operand resolution.

Clobbers may produce provenance, diagnostics, liveness sideband, or explicit
machine nodes when the target sequence requires them. They must not be treated
as broad public call-record decorations that later components reinterpret to
repair missing lowering.

## Direct And Indirect Callees

Direct callees resolve to typed function or symbol operands. Indirect callees
resolve to typed register, memory, or address operands supplied by the
prepared call plan and operand-resolution seam.

Indirect calls may require target-specific address materialization or a
register move before the call instruction. Those steps are call-adjacent and
owned here. Callee identity must not be recovered from source call expression
text, cached display strings, or rendered assembly.

## Memory Returns

Memory returns are ABI-owned call facts. Call lowering handles the hidden or
explicit return-address argument, validates the prepared destination memory
location, emits any required address setup before the call, and records the
result binding after the call.

The destination address comes through typed operand queries over prepared
memory-return, frame, spill, storage, or value-home facts. This file does not
invent memory-return slots from public compatibility records or source
aggregate spellings.

## Call-Site ABI Bindings

Call-site ABI bindings connect prepared semantic arguments and results to
target ABI locations. They are consumed as typed binding facts:

```cpp
struct CallSiteBinding {
  PreparedCallPlanId call_plan;
  size_t semantic_index;
  CallBindingRole role;
  AbiLocation location;
  Provenance provenance;
};
```

The exact implementation spelling can differ, but each emitted setup,
call, result, preservation, and clobber fact must trace back to the prepared
call plan and binding role. Public call records are projections of these facts
after canonical lowering, not semantic input.

## Emitted MachineInstruction Sequence

Every emitted instruction is a canonical MIR target node in the current
`MachineBlock`:

```cpp
MachineInstruction instruction {
  .opcode = selected_call_or_move_opcode,
  .operands = resolved_machine_operands,
  .provenance = provenance_for_call_plan(call_plan, sequence_role),
};
```

The canonical call sequence is ordered:

1. pre-call preserved-value setup required by the call plan
2. memory-return address setup when present
3. argument location moves and outgoing stack stores
4. direct or indirect call instruction
5. result capture from ABI result locations
6. post-call preserved-value restoration required by the call plan

The sequence may omit steps that the prepared call plan proves unnecessary.
It may also emit target-specific multi-instruction forms when the target
profile requires them. It must not lower into a flat compatibility vector or
ask compatibility projection to create the authoritative call sequence later.

## Operand Consumption

Call lowering consumes typed operands through narrow queries:

- semantic argument source operands
- ABI argument and result registers
- outgoing stack argument memory references
- direct callee symbols and relocation-aware forms
- indirect callee registers, memory references, and address operands
- memory-return destination and hidden pointer operands
- preserved-value source, temporary, and restore operands
- clobber registers and target register-class metadata

It may reject a typed operand that violates call ABI constraints. It must not
choose storage precedence, parse register strings, recover symbols from source
spellings, or inspect broad compatibility records while sequencing the call.

## Owned Inputs

- prepared call plans
- prepared argument and result locations
- prepared call-site ABI bindings
- prepared call-adjacent move facts
- prepared preserved-value facts
- prepared clobber facts
- prepared direct and indirect callee facts
- prepared memory-return facts
- current `FunctionLoweringContext`
- current `MachineBlock` insertion point
- typed `MachineOperand`, `MachineRegister`, symbol, memory, and ABI-location
  values from operand resolution
- target profile call, branch-with-link, indirect-call, register-class,
  outgoing-stack, clobber, and relocation constraints
- durable source-location references already linked to prepared identity for
  optional provenance

## Owned Outputs

- ordered target `MachineInstruction` nodes appended to canonical
  `MachineBlock` instruction storage
- canonical direct or indirect call instruction nodes
- ABI argument setup and result capture nodes owned by the call plan
- memory-return setup and result binding nodes
- preservation and clobber sideband or nodes when prepared call facts require
  them
- instruction provenance for call plan, binding role, argument index, result
  index, preserved value, clobber, and callee reason tags
- diagnostics for unsupported, ambiguous, missing, or contradictory call-plan
  authority
- private call facts needed by compatibility projection, when derived from
  canonical typed call lowering

## Indirect Queries

Call lowering may indirectly query:

- operand resolution for typed operands, ABI locations, target registers,
  memory references, symbols, relocation-aware callees, frame slots, spill
  slots, and memory-return destinations
- target profile metadata for call opcodes, direct and indirect call forms,
  register classes, stack argument alignment, clobber sets, and relocation
  forms
- function-context identity maps for provenance and diagnostics
- prepared call-plan, preserved-value, clobber, memory-return, and call-site
  ABI facts through narrow function-context services

It must not query ordinary instruction-lowering internals, branch/control
lowering internals, public assembly bridge output, compatibility projection
records, broad public call records, source BIR call spellings, or
`FunctionRecord::machine_nodes` while deciding call semantics.

## Forbidden Knowledge

`call_lowering.cpp` must not know:

- module-level public record assembly
- function or block traversal order beyond the active insertion point and
  function-context identity facts
- ordinary scalar ALU, memory, move, spill/reload, or parallel-copy opcode
  selection details outside call-owned adjacent moves
- terminator edge ownership, return control emission, or block sealing rules
- public assembly file structure, section traversal, punctuation, or spacing
- compatibility projection layout as semantic call authority
- broad public call records or inspection records as semantic inputs
- cached display strings as call, callee, register, operand, symbol, memory,
  argument, or result authority
- source spelling recovery as the primary callee or call-site identity model
- flat `FunctionRecord::machine_nodes` vectors as canonical call output
- target render APIs named `__repr__`

## Downstream Use

Function traversal appends the emitted call sequence to the current block.
Instruction lowering may later lower ordinary non-call operations, and
branch/control lowering may later lower the terminator. Neither component
owns call-site ABI sequencing. Compatibility projection may derive public call
records after canonical MIR exists by reading canonical call instructions,
typed operands, prepared ABI facts, and provenance.

The public assembly bridge and shared printer consume canonical
`MachineBlock` instructions; they must not ask this seam to reconstruct call
syntax or re-walk prepared call plans for printing.

## Replacement Rejection Signals

Reject an implementation of this draft if it:

- treats broad public call records, public inspection records, compatibility
  projections, or `FunctionRecord::machine_nodes` as call-lowering authority
- lowers argument or result placement by choosing among broad optional fields
  instead of consuming typed ABI locations through operand resolution
- recovers direct or indirect callee identity from source spellings, cached
  display strings, or rendered assembly
- moves call-site ABI sequencing into ordinary instruction lowering or
  branch/control lowering
- emits call instructions into a flat vector instead of canonical
  `MachineInstruction` nodes in `MachineBlock`
- lets compatibility projection synthesize the authoritative call sequence
  after the fact
- ignores prepared preserved-value, clobber, memory-return, indirect-callee,
  or call-site ABI binding facts when they are present
- introduces named-testcase call handling instead of lowering prepared call
  plans and typed operands semantically
- introduces target render APIs named `__repr__`
