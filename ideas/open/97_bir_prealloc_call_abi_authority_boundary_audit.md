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

