# Builtin VRM Register Carrier Types To Regalloc Frontier Runbook

Status: Active
Source Idea: ideas/open/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md
Activated from: ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md

## Purpose

Add source-level builtin VRM carrier types so inline asm vector-register
operands can become real compiler values before register allocation.

## Goal

Preserve `__c4c_builtin_vrm1`, `__c4c_builtin_vrm2`,
`__c4c_builtin_vrm4`, and `__c4c_builtin_vrm8` from source spelling through
HIR/LIR/BIR into pre-regalloc metadata that classifies the value as a vector
register group with the correct contiguous width.

## Core Rule

VRM carrier types describe only register footprint. Do not model element dtype,
lane count, storage layout, EV operation semantics, mask policy, or ordinary C
ABI behavior as part of this runbook.

## Read First

- `ideas/open/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md`
- `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`
- Existing inline asm constraint handling for `VR`, `VRM2`, `VRM4`, and any
  prepared inline asm carrier validation.
- Existing parser/type-system handling for builtin type spellings and typedef
  preservation.
- Existing HIR/LIR/BIR type lowering and dump tests.

## Current Targets

- Frontend parser and type-system surfaces that recognize builtin type names.
- Type identity paths for typedefs and C++ template substitution where ordinary
  type identity is already preserved.
- HIR/LIR lowering surfaces that currently choose scalar, pointer, struct, or
  GCC vector representations.
- BIR type or value metadata that can carry target-neutral vector register
  group width.
- Prepared inline asm carrier classification before register allocation.
- Focused dump, diagnostic, and inline asm smoke tests.

## Non-Goals

- Do not implement final register allocation placement unless an existing hook
  naturally consumes the new metadata.
- Do not implement `.insn.d` object emission, EV opcode layout, relocation,
  relaxation, disassembly, or linker behavior.
- Do not reuse GCC vector type metadata or add element dtype, lane count, or
  storage-layout semantics to VRM carriers.
- Do not add load/store intrinsics, EV compiler intrinsics, or compiler-known
  EV mnemonics.
- Do not make VRM carriers part of the ordinary C ABI in this runbook.
  Parameters, returns, globals, and memory storage may remain explicitly
  unsupported unless a narrow local representation already exists.
- Do not weaken scalar/VRM mismatch diagnostics or tests.

## Working Model

- Prefer a dedicated type carrier such as a small enum or equivalent structured
  representation instead of overloading `TypeSpec::is_vector`.
- Give the lowered form a stable visible spelling, for example `c4c.vrm1`,
  `c4c.vrm2`, `c4c.vrm4`, and `c4c.vrm8`, or an equivalent structured type
  dump that is not confusable with integers, pointers, structs, or GCC vectors.
- Publish pre-regalloc metadata as vector register class plus group width:
  `vrm1 -> 1`, `vrm2 -> 2`, `vrm4 -> 4`, `vrm8 -> 8`.
- Let inline asm constraints validate or reinforce width compatibility, but do
  not let constraint strings be the only source of vector identity.

## Execution Rules

- Inspect existing builtin type parsing, typedef identity, HIR/LIR/BIR lowering,
  and prepared inline asm carrier checks before choosing the representation.
- Keep each code slice behavior-preserving outside the VRM carrier surface.
- Add positive proof at each layer where identity could decay.
- Keep scalar negative coverage active; `"VRM2"(long_value)` must not become a
  valid vector operand.
- Use focused build and dump tests for each step. Escalate to the
  supervisor-selected broader subset when changes cross frontend, lowering, and
  backend preparation boundaries.

## Step 1: Map Existing Type And Inline Asm Carrier Surfaces

Goal: Identify the narrowest source-to-prealloc path for adding VRM carrier
identity without guessing at representation seams.

Primary targets:

- Parser/type-system builtin type spelling code.
- Typedef and C++ template type identity paths.
- HIR, LIR, BIR type representations and dump utilities.
- Prepared inline asm carrier validation for vector register constraints.

Actions:

- Inspect where builtin type names are recognized today.
- Inspect how ordinary typedef and template-substituted type identity survives
  into HIR/LIR/BIR.
- Locate where prepared inline asm decides operand register class and width.
- Record the selected representation and any unsupported ABI/storage surfaces
  in `todo.md`.

Completion check:

- The executor can name the exact source, HIR/LIR, BIR, and prealloc surfaces
  that must change.
- The chosen route does not rely on treating scalar values or GCC vector
  storage as VRM values.

## Step 2: Add Source Type Carrier And Frontend Proof

Goal: Parse the four builtin VRM type names as first-class type spellings and
preserve their identity through frontend type paths.

Primary targets:

- Source parser and type model for builtin type names.
- Typedef and supported C++ template substitution paths.
- Frontend or HIR dump tests.

Actions:

- Add distinct source type identity for `__c4c_builtin_vrm1`,
  `__c4c_builtin_vrm2`, `__c4c_builtin_vrm4`, and `__c4c_builtin_vrm8`.
- Preserve identity through typedef smoke cases.
- Preserve identity through already-supported template type substitution paths
  if they are in scope for the existing frontend.
- Reject or explicitly diagnose unsupported ABI/storage uses if they would
  otherwise silently decay.

Completion check:

- Focused frontend/HIR proof shows each builtin and a typedef case retain VRM
  identity.
- Dumps make clear the type is not `int`, `long`, a pointer, a struct, or a GCC
  vector fallback.

## Step 3: Preserve VRM Identity Through LIR/BIR Lowering

Goal: Carry the VRM identity through lowering into a stable internal
representation before backend preparation.

Primary targets:

- HIR-to-LIR type lowering.
- LIR-to-BIR or equivalent backend-facing lowering.
- LIR/BIR dump tests.

Actions:

- Add a stable lowered spelling or structured type reference for each VRM
  width.
- Ensure lowering preserves width `1`, `2`, `4`, and `8`.
- Keep the representation target-neutral: vector register group width, not EV
  instruction semantics.
- Add tests that would fail if the type decays to scalar, pointer, struct, or
  GCC vector storage.

Completion check:

- LIR/BIR proof shows `c4c.vrm1`, `c4c.vrm2`, `c4c.vrm4`, and `c4c.vrm8`, or
  an equivalent unambiguous structured representation.
- No proof depends on fixture-built prepared modules while source-level
  lowering remains absent.

## Step 4: Publish Pre-Regalloc Classification Metadata

Goal: Make VRM values visible to prepared inline asm and regalloc-frontier code
as vector register groups with the correct width.

Primary targets:

- BIR or prealloc metadata consumed before register allocation.
- Prepared inline asm carrier classification.
- Liveness/prealloc/regalloc-frontier dumps.

Actions:

- Classify VRM values as `PreparedRegisterClass::Vector` or the existing
  equivalent vector class.
- Publish group width from the source type: `1`, `2`, `4`, or `8`.
- Validate matching inline asm constraints using the value metadata.
- Preserve scalar/vector mismatch diagnostics for non-VRM operands.

Completion check:

- Prealloc/liveness/regalloc-frontier proof shows each VRM width maps to vector
  class with the expected group width.
- A matching VRM inline asm smoke reaches the prepared carrier frontier without
  `operand_home_incompatible_register_class`.
- A scalar negative case such as `"VRM2"(long_value)` still rejects or reports
  incompatibility.

## Step 5: Validate Frontier And Prepare Handoff

Goal: Prove the source-to-regalloc-frontier chain is complete and leave final
allocation/substitution/object-byte work to follow-up ideas.

Actions:

- Run focused build and tests for parser/type, HIR/LIR/BIR dumps, prepared
  inline asm carrier validation, and scalar negative diagnostics.
- Run the supervisor-selected broader subset if the implementation touched
  shared frontend or backend lowering surfaces.
- Check that no test was weakened, reclassified, or made dependent on a named
  fixture shortcut.
- Summarize proof and any explicit follow-up needs in `todo.md`.

Completion check:

- Acceptance criteria from the source idea are met through the regalloc
  frontier.
- Final register allocation, substitution, `.insn.d` encoding, and object-byte
  proof remain outside this runbook and are ready for a focused follow-up idea.
