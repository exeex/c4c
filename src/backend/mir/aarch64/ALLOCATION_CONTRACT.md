# AArch64 Allocation Contract

This contract defines the BIR/prepared-to-target-MIR allocation boundary for
AArch64. It is the authority for register budgets, physical value homes,
spill-slot identity, reserved scratch use, and portable allocation metadata
before AArch64 target MIR records become machine instruction nodes.

## Pipeline Position

The accepted lowering path is:

```text
BIR / PreparedBirModule
  -> target ABI register-budget contract
  -> allocation result
  -> AArch64 target MIR records
  -> AArch64 machine instruction nodes
  -> optional .s printer output or structured encoder/object input
```

Core BIR owns semantic value identity. `PreparedBirModule` owns target-facing
facts such as value homes, liveness, register classes, frame plans, call plans,
move plans, ABI bindings, and spill/reload facts. This allocation contract owns
the target ABI resource taxonomy and the allocation-result shape consumed by
AArch64 target MIR. The later machine-instruction-node contract owns selected
instruction-like operations and downstream printer/encoder handoff.

Assembly text, rendered register names, and final stack-offset strings are not
allocation authority. They may be spelling or diagnostic output after the
structured allocation result has already chosen a resource.

## Register Classes And Budgets

AArch64 allocation resources are finite physical registers partitioned by
register class:

- GPR class: `x0` through `x30` with 64-bit `x` and 32-bit `w` views. `sp` is a
  stack-pointer resource, not a general-purpose value register.
- FPR/SIMD class: `v0` through `v31` with scalar and vector views such as `s`,
  `d`, `q`, and `v`.

The GPR and FPR/SIMD pools must remain separate. A prepared general or
aggregate-address value cannot be allocated from the FPR/SIMD pool, and a
prepared float or vector value cannot be allocated from the GPR pool except
through an explicit later lowering rule that changes the value representation.

Register views are not independent allocation units. `x9` and `w9` refer to
the same GPR resource with different width effects. `s0`, `d0`, `q0`, and `v0`
refer to the same FPR/SIMD resource with different operand views.

## AAPCS64 Pool Taxonomy

For the AAPCS64 AArch64 path, the target ABI register-budget contract exposes
these pools. The lists are finite and target-specific.

### GPR Pools

- Argument/return GPRs: `x0` through `x7`. `x8` is the indirect result-location
  register for memory returns and may be used only when the prepared call plan
  or ABI binding authorizes that role.
- Caller-saved temp GPRs: `x0` through `x18` and `x30`, except that registers
  with a special role below are unavailable unless that role is explicitly in
  force. `x16` and `x17` are intra-procedure-call or linker/veneer scratch
  registers and are not ordinary long-lived homes.
- Callee-saved GPRs: `x19` through `x29`. `x29` is the frame pointer when the
  prepared frame plan requires a frame pointer and is then not allocatable for
  a long-lived BIR value.
- Allocatable long-lived GPR candidates: registers accepted by the allocation
  result for prepared GPR or aggregate-address values after excluding special,
  forbidden, and reserved MIR scratch registers. Prefer non-special
  callee-saved or caller-saved registers according to liveness and call
  clobber costs; this contract does not prescribe the allocator algorithm.
- Reserved MIR scratch GPRs: a target-MIR-owned subset kept unavailable for
  long-lived homes. First-pass AArch64 spill/reload planning should reserve
  `x9` and `x10` for ordinary GPR materialization and keep `x16` and `x17`
  available only for indirect-call, veneer, or explicitly modeled call-lowering
  needs.
- Special or forbidden GPR resources: `sp`, `x29` when used as frame pointer,
  `x30` when used as link register, `x18` as the platform register, `x8` when
  used for indirect result location, and `x16`/`x17` when the call/linker
  veneer role owns them.

### FPR/SIMD Pools

- Argument/return FPR/SIMD registers: `v0` through `v7`.
- Caller-saved temp FPR/SIMD registers: `v0` through `v7` and `v16` through
  `v31`.
- Callee-saved FPR/SIMD registers: `v8` through `v15`. Only the ABI-preserved
  portions are callee-save resources; the allocation result must retain the
  required view and width metadata so later save/restore planning is correct.
- Allocatable long-lived FPR/SIMD candidates: registers accepted by the
  allocation result for prepared float or vector values after excluding any
  reserved MIR scratch registers.
- Reserved MIR scratch FPR/SIMD registers: a target-MIR-owned subset kept
  unavailable for long-lived homes. First-pass AArch64 spill/reload planning
  should reserve `v16` and `v17` for ordinary FPR/SIMD materialization.
- Special or forbidden FPR/SIMD resources: any register whose role is fixed by
  a prepared ABI binding, helper-call convention, inline-asm constraint, or
  later accepted target-specific rule for the duration of that operation.

## Allocation Result Shape

The allocation result maps each prepared/BIR value that needs a target location
to one of these structured locations:

- Physical register location: a typed AArch64 register reference with bank,
  view, index, register class, prepared value id, optional value name id, type
  kind, and the prepared fact that authorized the assignment.
- Structured spill-slot location: a target allocation spill-slot id, size,
  alignment, register class, owning function, prepared value provenance, and
  lifetime or interval metadata when available.
- Future virtual-register placeholder: a typed virtual id with register class,
  view or width, provenance, and an explicit owner naming the later allocator
  or lowering stage that must resolve it before physical machine instruction
  nodes are emitted.
- Non-register prepared location: a constant, symbol, frame slot, memory
  return location, aggregate storage plan, or other already prepared
  non-register location.

Physical register locations are finite because the target ABI exposes a finite
register budget. Spill-slot locations are logically unbounded at this contract
level. Running out of physical registers is represented by assigning additional
values to structured spill-slot ids, not by weakening BIR semantics, inventing
rendered register names, or stealing reserved scratch registers.

## Spill Slots

A spill slot is a structured allocation id. It must carry:

- function identity
- spill-slot id unique within that function's allocation result
- register class and required view or width
- size and alignment
- source prepared value id, value name id, or move/spill provenance when known
- fixed-location policy only when an upstream prepared frame fact requires it

Spill slots are not stack-pointer offsets and are not rendered address strings.
Frame layout may later assign concrete offsets and final addressing forms, but
target MIR must preserve the spill-slot id as the authority until that point.

## Reserved MIR Scratch Policy

Reserved MIR scratch registers exist so target MIR can materialize spilled
operands and stores before a full register scavenger exists. They are excluded
from long-lived BIR value homes.

For a reload, target MIR should choose a scratch register from the spilled
value's register class, emit or record a reload pseudo operation from the
spill-slot id into that scratch register, and use the scratch register only for
the bounded operation or sequence that needed the value. For a store, target
MIR should choose a scratch register of the result class when the result is not
already in a storable physical register, then emit or record a store pseudo
operation back to the spill-slot id.

If the reserved scratch pool for the required class is exhausted, the first
implementation must fail closed or open a separate register-scavenger idea. It
must not silently take a long-lived register or reinterpret an argument/return
register as scratch without a structured clobber and liveness proof.

## Call-Clobber And Callee-Save Obligations

The allocation result must distinguish caller-saved, callee-saved,
argument/return, reserved scratch, and special resources at every call
boundary.

Calls consume prepared call plans for direct, indirect, variadic, memory-return,
argument, result, preserved-value, and clobber facts. A value live across a
call may remain in a caller-saved register only if the allocation result also
records the preservation strategy, such as a spill slot, a move to a
callee-saved register, or another prepared call-preservation fact.

Callee-saved register use creates an obligation for the frame/prologue plan to
save and restore the relevant physical resource. This contract records that
obligation; it does not implement prologue/epilogue emission or concrete frame
layout.

Indirect calls and linker/veneer-sensitive operations must preserve the
special roles of `x16` and `x17`. Memory-return lowering must preserve the
special role of `x8`. Platform use of `x18`, frame-pointer use of `x29`, and
link-register use of `x30` must be carried as special resources, not ordinary
allocation choices.

## Reload And Store Pseudo Plan Placement

Core BIR must not own AArch64 reload/store instructions. The allocation result
may state that a value lives in a spill slot, and prepared facts may carry
spill/reload plans, but target MIR is the first layer that introduces
AArch64-specific reload/store pseudo operations or an equivalent structured
lowering plan for materializing spilled operands.

Those pseudo operations are pre-machine-node target MIR facts. Later
instruction selection may turn them into structured AArch64 machine instruction
nodes and then into printer or encoder output. Assembly text is never the
handoff for reload/store semantics.

## Portable x86/x86-64 Correctness Carriers

Although this contract is stored under the AArch64 backend, its allocation
result shape must remain portable enough for a conservative x86/x86-64 backend
that lowers in a simple RISC-like style first. The portable carrier must be
able to represent:

- fixed input and output registers, including accumulator or dividend pairs
- implicit operands and implicit clobbers
- flags or condition-code resources as target resources with def/use/clobber
  metadata
- variable-count shift resources such as count registers
- subregister aliases and width effects, including writes that clear, preserve,
  or partially update a larger physical resource
- target ABI variants such as SysV and Windows x86-64 argument/return
  registers, shadow space, red zone, stack alignment, and call-preserved sets
- fixed stack resources and ABI-owned outgoing argument areas

The first x86/x86-64 implementation may ignore CISC optimization opportunities
such as memory-operand ALU folding, complex addressing, or partial-register
optimization. It may not ignore correctness carriers for fixed operands,
flags, aliases, width effects, clobbers, or ABI stack resources.

## Rejected Routes

The following routes are rejected:

- storing AArch64 physical-register names or final stack offsets as core BIR
  semantic truth
- collapsing GPR and FPR/SIMD values into a single untyped allocation pool
- treating caller-saved temp registers and reserved MIR scratch registers as
  interchangeable
- letting scalar ALU, memory, branch, call, return, vector, or inline-asm
  lowering independently allocate registers or invent spill slots
- representing spill slots as rendered stack-address strings before frame
  layout owns concrete offsets
- emitting reload/store behavior as assembly text before target MIR pseudo
  operations or machine instruction nodes exist
- silently stealing long-lived registers when reserved scratch is exhausted
- redefining assembler, encoder, or object-writer handoff instead of consuming
  the later machine-instruction-node contract

## Relationship To Existing AArch64 Surfaces

`BACKEND_ENTRY_CONTRACT.md` owns the requirement that AArch64 lowering consumes
`PreparedBirModule` rather than raw BIR or rendered text. This allocation
contract names the allocation-resource boundary inside that prepared handoff.

`abi/abi.hpp` owns typed AArch64 register references and conversion from
prepared physical-register spellings. Those helpers expose target register
facts; they do not allocate values by themselves.

`module/module.hpp` owns target MIR snapshots of prepared value homes,
regalloc assignments, storage plans, frame slots, call plans, ABI bindings,
callee-save and clobber records, moves, and spill/reload operations. Display
strings and concrete offsets in those records are snapshots or diagnostics
only unless a later frame-layout contract says otherwise.

`codegen/records.hpp` owns target MIR record variants consumed before machine
instruction nodes. Record construction may consume the allocation result, but
it must not recover allocation facts from assembly text or rendered names.

`MACHINE_INSTRUCTION_NODE_CONTRACT.md` owns the next boundary after target MIR.
This document does not authorize instruction selection, printing, encoding,
object writing, linking, or a full register allocator.
