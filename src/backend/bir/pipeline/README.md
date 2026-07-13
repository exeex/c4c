# Ordered Backend Pipeline Contract

Status: scaffold.

This file is the single authority for stage order. Individual documents define
local behavior but cannot reorder themselves.

## Phase A: import and raw verification

1. `lir_to_bir` constructs `RawBir` through builders.
2. Raw verification checks ownership, stable IDs, types, operand roles,
   terminators, def-use, and documented raw-only forms.

No canonical pass may execute on an unverified `RawBir`.

## Phase B: canonical BIR passes

Passes execute in this order:

1. [`legalize`](../passes/legalize/README.md)
2. [`scalar`](../passes/scalar/README.md)
3. [`cfg`](../passes/cfg/README.md)
4. [`ssa`](../passes/ssa/README.md)
5. [`memory`](../passes/memory/README.md)
6. [`aggregate`](../passes/aggregate/README.md)
7. [`intrinsics`](../passes/intrinsics/README.md)
8. Canonical verification and `CanonicalBir` publication

The order is initially conservative. A later review may split a pass into
numbered subpasses, but it must update this file and both adjacent contracts in
the same design change.

## Phase C: preparation

Preparation reads immutable `CanonicalBir` and produces typed plans in this
order:

1. target context and data-layout selection
2. [`abi`](../preparation/abi/README.md)
3. [`calls`](../preparation/calls/README.md)
4. [`variadic`](../preparation/variadic/README.md)
5. [`address`](../preparation/address/README.md)
6. [`inline_asm`](../preparation/inline_asm/README.md)
7. [`runtime_helpers`](../preparation/runtime_helpers/README.md)
8. Prepared-input verification and `PreparedBir` publication

Planners may depend on earlier typed plans. They may not communicate through
unnamed mutable side tables or alter canonical instructions.

## Phase D: MIR construction and passes

MIR work executes in this order:

1. [`out_of_ssa`](../mir/passes/out_of_ssa/README.md)
2. [`instruction_select`](../mir/passes/instruction_select/README.md)
3. [`call_lowering`](../mir/passes/call_lowering/README.md)
4. [`register_allocation`](../mir/passes/register_allocation/README.md)
5. [`spill_reload`](../mir/passes/spill_reload/README.md)
6. [`frame_layout`](../mir/passes/frame_layout/README.md)
7. [`prologue_epilogue`](../mir/passes/prologue_epilogue/README.md)
8. MIR verification and target emission

The relative position of instruction selection and register allocation is an
explicit review checkpoint. It is recorded here so it cannot remain an
accidental property of implementation order.

## Pass execution contract

Each mutating pass receives an exclusive editor and returns a mutation summary,
preserved analyses, and structured diagnostics. A pass must leave its output
valid for the next named pass or fail transactionally. Debug/test pipelines
verify after every mutating pass; publication always verifies.
