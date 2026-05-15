# Prepared I128 Runtime Helper Authority

Status: Open
Created: 2026-05-15

Parent Context: ideas/open/236_aarch64_i128_pair_lowering.md

## Goal

Expose prepared/shared runtime-helper boundary facts for i128 operations that
cannot be lowered as direct AArch64 pair machine nodes, so idea 236 can consume
structured helper records without target-local helper selection or fixed
register marshaling.

## Why This Exists

Step 6 of the AArch64 i128 pair lowering runbook found that helper-required
i128 operations, such as div/rem and float/i128 conversions, are still
represented as scalar `bir::BinaryInst` or `bir::CastInst` operation shapes.
Generic retained-call facts are only useful after a real call boundary exists;
they do not map source i128 operation identity to a helper family, callee,
low/high argument lanes, low/high result lanes, memory-return ownership, or
helper-specific clobber/resource policy.

Continuing directly in AArch64 target lowering would force target codegen to
invent helper selection, ABI marshaling, lane ownership, or clobbers from
opcodes, rendered register names, or fixed `x0`/`x1` accumulator conventions.

## In Scope

- Define prepared/shared helper authority for i128 operations that require
  runtime calls.
- Map source operation identity to helper kind and callee symbol for supported
  i128 helper families.
- Preserve low/high argument lane bindings and low/high result lane bindings
  as structured facts.
- Preserve memory-return ownership where the helper ABI returns through
  memory rather than direct low/high result lanes.
- Preserve helper-specific clobber and resource policy, including ABI and
  register-bank transition facts needed by selected AArch64 consumers.
- Add focused observations or tests proving helper authority is structural and
  incomplete states fail closed.

## Out Of Scope

- Do not add AArch64 selected helper-call machine nodes or printer output in
  this prerequisite; idea 236 owns consuming these facts after this route
  lands.
- Do not create a local i128 allocator or helper ABI marshaler inside AArch64
  target lowering.
- Do not lower i128 helper-required operations as scalar i64 operations.
- Do not use fixed `x0`/`x1` accumulator pairs, rendered register adjacency, or
  named testcase shortcuts as helper authority.
- Do not broaden into binary128 soft-float, scalar FP, atomics, intrinsics,
  inline asm, callee-save placement, or preserved-value extent work.

## Acceptance Criteria

- Supported helper-required i128 operations expose a prepared/shared mapping
  from source operation identity to helper kind and callee symbol.
- Helper boundary facts expose structured low/high argument lane bindings and
  low/high result lane bindings, or explicit memory-return ownership when
  required by the helper ABI.
- Helper-specific clobbers, resources, ABI facts, and register-bank
  transitions are explicit enough for idea 236 to consume without target-local
  policy.
- Incomplete helper authority fails closed with explicit diagnostics instead
  of being patched in AArch64 dispatch or printer code.
- Idea 236 can resume Step 6 with selected helper records consuming these facts
  rather than synthesizing helper calls from `BinaryInst` or `CastInst`
  opcodes.

## Reviewer Reject Signals

- The route adds AArch64-side helper selection from `BinaryInst` or `CastInst`
  opcodes instead of prepared/shared helper authority.
- The route hard-codes helper callee names, fixed `x0`/`x1` lane ownership,
  register adjacency, or clobber policy inside AArch64 target lowering.
- The route lowers helper-required i128 operations as scalar i64 shortcuts or
  claims progress through one named fixture.
- The route weakens unsupported expectations or changes diagnostics while the
  same missing helper kind, callee, lane, memory-return, clobber, or resource
  facts remain absent.
- The route renames existing call carriers or relies only on generic
  `PreparedCallPlan` facts without preserving source i128 operation identity
  and low/high lane authority.
- The route broadens into binary128, scalar FP, atomic, intrinsic, inline-asm,
  callee-save, preserved-value extent, or unrelated frame/call rewrites.
