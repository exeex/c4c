# BIR Prealloc Call ABI Authority Boundary Audit

## Goal

Analyze the boundary between BIR call ABI facts and prealloc call-plan/runtime
helper inference, then produce follow-up ideas only for duplicated authority or
unclear contracts found during the audit.

## Why This Exists

Before rebuilding x86 and RISC-V backend routes, the shared BIR/prealloc
foundation needs a clean authority split. BIR `CallInst` and `Function` already
carry call-related facts such as argument ABI, result ABI, calling convention,
direct callee identity, and typed call metadata. Prealloc also infers and
classifies call ABI facts while producing prepared call plans, runtime-helper
plans, variadic entry plans, and special-carrier plans.

The risk is that BIR prepares a call fact and prealloc reconstructs the same
fact from raw BIR shape or target profile. Some duplication may be intentional:
target-sensitive ABI legalization belongs in prealloc. The audit should
separate intentional legalization from accidental re-derivation.

## Owned Files

- Audit/read:
  - `src/backend/bir/bir.hpp`
  - `src/backend/bir/lir_to_bir/calling.cpp`
  - `src/backend/bir/lir_to_bir/call_abi.cpp`
  - `src/backend/bir/lir_to_bir/module.cpp`
  - `src/backend/prealloc/legalize.cpp`
  - `src/backend/prealloc/call_plans.cpp`
  - `src/backend/prealloc/calls.hpp`
  - `src/backend/prealloc/variadic_entry_plans.cpp`
  - `src/backend/prealloc/i128_runtime_helpers.cpp`
  - `src/backend/prealloc/f128_runtime_helpers.cpp`
  - `src/backend/prealloc/regalloc/call_moves.cpp`
- No implementation files should be edited by this idea.

## In Scope

- Inventory call ABI facts produced by BIR and name their intended authority.
- Inventory call ABI, wrapper, result, variadic, i128, f128, and call-move
  inference/classification done by prealloc.
- Classify each overlap as:
  - `intentional-prealloc-legalization`
  - `bir-fact-consumed-correctly`
  - `prealloc-rederives-bir-fact`
  - `bir-missing-target-neutral-fact`
  - `contract-ambiguous`
  - `needs-follow-up-idea`
- Produce follow-up ideas only for concrete overlaps with file ownership and a
  proof route.

## Out Of Scope

- Implementing call ABI cleanup.
- Changing target ABI behavior, call-plan layout, runtime-helper behavior, or
  test expectations.
- Moving target-sensitive ABI legalization into BIR.
- Reopening AArch64 calls cleanup routes.

## Proof Expectations

- This is analysis-only; no backend tests are required unless implementation
  files are accidentally changed.
- The close note must name any generated follow-up ideas and explicitly list
  overlaps judged intentional.

## Reviewer Reject Signals

- The audit proposes implementation before proving duplicate authority.
- It treats all ABI inference as duplication without separating target-sensitive
  legalization from BIR semantic facts.
- It creates vague "clean calls" follow-ups without naming exact BIR/prealloc
  facts.

## Close Note

Closed after the active audit runbook completed all five steps. The audit
classified BIR/prealloc call ABI overlaps and generated focused follow-up
ideas for the concrete gaps:

- `ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md`
- `ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md`
- `ideas/open/102_aapcs64_va_arg_payload_shape_authority.md`
- `ideas/open/103_prealloc_synthetic_helper_call_abi_authority.md`

Retained `bir-fact-consumed-correctly` overlaps:

- Structured direct/indirect callee identity and indirect callee operands are
  consumed for wrapper selection and indirect source planning.
- Present call arg ABI, arg types, operand order, byval/sret flags, result ABI,
  return ABI, memory-return storage, byval aggregate facts, fixed HFA pressure
  facts, and variadic markers are consumed by prealloc planning and move
  resolution.
- Aggregate AAPCS64 `va_arg` planning consumes BIR size/align and memory
  carrier facts, with only naming ambiguity left unpromoted.

Retained `intentional-prealloc-legalization` overlaps:

- Physical arg/result register and stack placement, byval copy chunks,
  register-bank presentation, call/return moves, clobbers, preserved values,
  and boundary effects remain prealloc authority.
- Variadic wrapper details, SysV FPR counts, AAPCS64 save-area planning, and
  named variadic register counts remain target-sensitive prealloc authority
  when they consume present BIR facts.
- Prealloc mutation of BIR ABI carriers in `legalize.cpp` for target-facing
  promotion and size/align repair is retained as the main authority-boundary
  exception.
- I128/F128 carrier homes, lane/full-width marshaling, helper callee
  selection, helper clobbers, preserved values, boundary effects, and
  live-preservation completeness remain prealloc/runtime-helper authority.
