# AArch64 Backend Entry Contract

This contract defines the accepted entry boundary for rebuilding the AArch64
backend after the markdown-first extraction. It is scoped to the backend input
and target-local facts required before instruction selection or assembly
generation resumes.

## Accepted Entry Type

The AArch64 backend entry must consume
`c4c::backend::prepare::PreparedBirModule`.

The public or driver-facing compatibility route may still start from
`bir::Module` or LIR, but that route must run the shared semantic BIR and
prepare pipeline first. A target-local AArch64 lowering stage starts only after
`prepare_semantic_bir_module_with_options(...)` has produced a
`PreparedBirModule` for an AArch64 `TargetProfile`.

Raw `bir::Module` is not accepted as the target-local AArch64 lowering input.
It is a valid upstream semantic representation and remains useful for dumps,
validation, and shared preparation, but it is too early for the AArch64 backend
entry because it does not by itself carry the prepared frame, dynamic stack,
addressing, liveness, allocation, storage, call, and control-flow plans that
target lowering needs.

## Required Prepared Facts

An accepted `PreparedBirModule` must provide these facts to the AArch64 route:

- `module`: the semantic BIR module retained for operation, type, global,
  string, function, block, and instruction structure.
- `target_profile`: an AArch64 target profile, including the AAPCS64 backend
  ABI where the target triple resolves to that ABI.
- `names`: prepared intern tables for function names, block labels, value
  names, slot names, and link names.
- `control_flow`: prepared blocks, branch conditions, join transfers, and
  parallel-copy bundles. AArch64 must use these records for branch and former
  phi semantics instead of reconstructing joins from rendered labels.
- `stack_layout`, `frame_plan`, and `dynamic_stack_plan`: frame size,
  alignment, frame slots, saved callee registers, fixed-slot frame-pointer
  policy, and dynamic-alloca / stack-save / stack-restore requirements.
- `addressing`: memory accesses keyed by prepared function/block/instruction
  identity, including frame-slot, global-symbol, pointer-value, and string
  constant bases.
- `liveness`, `register_group_overrides`, `regalloc`, `value_locations`, and
  `storage_plans`: value identity, register class or bank decisions, physical
  register or stack-slot homes, move bundles, spill/reload facts, and storage
  encodings already prepared by the shared backend pipeline.
- `call_plans`: direct, extern, variadic, indirect, memory-return, argument,
  result, preserved-value, and clobber facts for call lowering.
- `invariants` and `completed_phases`: proof that the configured prepare
  phases ran and that target-facing invariants such as no target-facing `i1`
  and no phi nodes are available when those phases are enabled.

If one of these facts is missing or ambiguous for an AArch64 feature, the
backend must stop at the contract boundary and record the gap. It must not
invent a target-local workaround that recovers the fact from printed BIR,
rendered assembly, legacy LIR type strings, or old AArch64 markdown examples.

## Raw `bir::Module` Narrowing Decision

The only permitted raw `bir::Module` role is an upstream staging input that is
immediately converted into `PreparedBirModule`. A narrower raw-BIR-only AArch64
entry is rejected for this rebuild.

The concrete reason is that current raw BIR intentionally separates semantic
program structure from prepared backend obligations. Raw BIR carries structured
operation and declaration data, and many fields already carry semantic ids, but
the target backend still needs later prepared facts:

- frame objects and frame-slot offsets
- fixed and dynamic stack policy
- branch-condition and join-transfer authority after phi removal
- live intervals, interference, register constraints, and move resolution
- value homes, storage plans, ABI argument/result plans, preserved values, and
  clobbers
- prepared address bases and access records

Accepting raw BIR at the AArch64 lowering boundary would force the target to
duplicate shared preparation or to rediscover prepared facts by matching names
and shapes. That would repeat the old route's failure mode. A future staged
subset may be introduced only if it is a typed subset of `PreparedBirModule`
with explicit ownership of the same facts, not a fallback to raw BIR.

## Semantic Identity

Semantic identity must flow through structured ids, with display strings used
only for final spelling, dumps, diagnostics, or target register names.

AArch64 lowering must treat these ids as authoritative where valid:

- `FunctionNameId` and `LinkNameId` for functions, direct callees, globals,
  pointer symbols, and externally visible symbols.
- `BlockLabelId` for blocks, branch targets, phi incoming labels, prepared
  branch conditions, join transfers, and parallel-copy bundles.
- `ValueNameId` and `PreparedValueId` for value homes, liveness, regalloc,
  storage plans, call plans, memory accesses, and move bundles.
- `SlotNameId`, `PreparedObjectId`, and `PreparedFrameSlotId` for stack
  objects, local slots, sret storage, frame slots, and prepared address bases.
- `TextId` for string constants where BIR lowering provides structured string
  identity.

Fields such as BIR value names, block label spellings, global names, string
names, register names, and emitted symbol spellings are not lookup authority
when a structured id is available. Physical register strings inside prepared
records describe target-local physical resources after shared preparation; they
do not replace semantic value identity.

## Target-Local MIR, Machine Nodes, And Assembly Needs

Before AArch64 instruction selection resumes, the target must define a
target-local MIR boundary that consumes `PreparedBirModule` facts without
rendering assembly text as an intermediate semantic form. The minimum target
structures are:

- an AArch64 module/function/block MIR container keyed back to prepared
  function and block ids
- typed virtual or prepared-value operands that retain `PreparedValueId`,
  `ValueNameId`, and `TypeKind` where applicable
- target register classes and physical register references separated from
  semantic value ids
- frame, stack-slot, dynamic-stack, and callee-save records sourced from the
  prepared frame and stack plans
- memory operands that preserve prepared address base kind, frame-slot id,
  symbol `LinkNameId`, pointer value id/name, string identity, offset, size,
  alignment, volatility, and address-space facts
- branch and compare records sourced from `control_flow.branch_conditions` and
  target labels sourced from `BlockLabelId`
- call records sourced from `call_plans`, including wrapper kind, direct or
  indirect callee identity, argument/result placements, memory-return storage,
  variadic FP-register counts, preserved values, and clobbers
- move, copy, spill, reload, and ABI-binding records sourced from
  `value_locations`, `regalloc`, `storage_plans`, and prepared control-flow
  parallel-copy bundles
- a data/object side table for globals, string constants, symbol visibility,
  TLS, constants, initializers, and later relocation needs

Instruction selection must publish structured AArch64 machine instruction
nodes derived from those MIR records. Assembly generation may be added only as
a `.s` printer over those nodes, and a built-in encoder or object writer may
consume only those nodes or a lower structured encoding record derived from
them. Parser recovery from printed assembly text is not an accepted bridge
between codegen and encoding/object emission. A string-emitting codegen
function must not become the first rebuilt AArch64 lowering API.

## Rejected Routes

The AArch64 rebuild explicitly rejects these routes:

- rendered-name recovery for function, block, value, slot, string, global, or
  callee identity when a structured id is expected
- string fallback routes that parse printed BIR, LIR type strings, legacy
  assembly text, or markdown examples to recover backend facts
- direct assembly-text emission as the primary lowering representation
- legacy shape recognizers that make narrow BIR/LIR patterns pass without a
  general semantic lowering rule
- parser/assembler operand recovery as a substitute for target-local MIR
  operands
- reviving `ArmCodegen` or old AArch64 direct text-emitter entry points as the
  new backend contract
- built-in assembler or linker orchestration as an implied requirement of the
  backend entry boundary

These exclusions align with `CLASSIFICATION_INDEX.md`, which marks rendered
assembly recovery, old direct emitters, parser operand recovery, and built-in
linker orchestration as contract exclusions for the rebuild.

## Relationship To The Classification Index

`CLASSIFICATION_INDEX.md` is the triage guide for the extracted AArch64
markdown artifacts. This file is the accepted Step 4 backend-entry contract.

Use the index to decide which legacy notes can inform later target-ABI,
instruction-selection, assembler, or binary-utils work. Do not use the index,
or any extracted legacy markdown file, as proof that live AArch64 lowering,
assembly, object emission, or linking exists.
