# AArch64 AAPCS64 Call Return Frame MIR Contract

Status: Open
Created: 2026-05-13

Depends On:
- `ideas/closed/211_aarch64_machine_instruction_node_contract.md`
- `ideas/closed/212_bir_mir_allocation_contract.md`
- `ideas/closed/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md`

## Goal

Define the AArch64 AAPCS64 target-MIR contract for calls, returns, stack frame
obligations, callee-save preservation, memory returns, indirect calls, and
prologue/epilogue inputs before implementing call/return instruction selection.

## Why This Idea Exists

The BIR/prepared ledger marks AAPCS64 call/register completeness as ambiguous.
Prepared call plans and allocation facts exist, but target MIR still needs an
accepted contract that says how AArch64 consumes them for call boundaries and
frame obligations.

This should be resolved before growing real calls, returns, or prologue code.
Otherwise each lowering slice will make local assumptions about argument
registers, result registers, stack arguments, `x8`, `x16/x17`, callee-saves,
scratch registers, and frame layout.

## In Scope

- Define a markdown contract under `src/backend/mir/aarch64/` for AAPCS64
  target-MIR responsibilities:
  - direct and indirect call records
  - argument and return binding records
  - memory-return handling and `x8` ownership
  - `x16/x17` indirect-call, PLT, veneer, and linker-sensitive roles
  - caller-saved clobber records
  - callee-save obligations and frame/prologue inputs
  - stack arguments, stack alignment, and outgoing call area facts
  - variadic deferral or accepted minimum contract
  - return-value and return-control records
- Audit existing AArch64 ABI/module/codegen markdown and live record surfaces
  for call/return/frame assumptions.
- Decide which prepared carriers are sufficient and which missing facts need
  separate `ideas/open/` gap ideas.
- Update `BIR_PREPARED_GAP_LEDGER.md` so calls, returns, frame obligations, and
  prologue/epilogue preconditions have an explicit next owner.

## Out Of Scope

- Emitting `bl`, `blr`, `ret`, stack adjustment, save/restore, or any final
  machine instruction nodes.
- Implementing frame layout or concrete stack offsets.
- Implementing full variadic, aggregate return, exception, TLS, PLT/GOT, or
  linker behavior unless only documenting required deferral.
- Rewriting shared preparation algorithms.
- Using legacy assembly-text emitter behavior as the authority.

## Acceptance Criteria

- A committed AArch64 contract clearly defines the target-MIR call, return, and
  frame boundary over prepared call/allocation/frame facts.
- The contract explicitly handles or defers direct calls, indirect calls,
  memory returns, return registers, stack arguments, caller/callee-save
  obligations, `x8`, `x16/x17`, `x29`, `x30`, and reserved MIR scratch.
- Missing BIR/prepared carriers discovered during the work are opened as
  separate ideas rather than bypassed in AArch64.
- Later instruction-selection slices can rely on this contract instead of
  rediscovering AAPCS64 policy locally.

## Reviewer Reject Signals

- Call, return, or prologue instruction selection starts before the contract is
  accepted.
- The route treats old text-emitter calling convention choices as current
  backend authority.
- Special registers such as `x8`, `x16/x17`, `x29`, or `x30` are treated as
  ordinary allocatable registers.
- Variadic, memory-return, indirect-call, or stack-argument gaps are hidden
  instead of explicitly accepted, deferred, or split.
