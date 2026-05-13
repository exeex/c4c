# AArch64 AAPCS64 Call Return Frame MIR Contract

Status: Active
Source Idea: ideas/open/214_aarch64_aapcs64_call_return_frame_mir_contract.md

## Purpose

Define the AArch64 AAPCS64 target-MIR contract for call boundaries, return
boundaries, frame obligations, and prologue/epilogue inputs before any final
call, return, or prologue instruction selection starts.

## Goal

Make later AArch64 call, return, frame, and machine-node selection slices
consume an accepted target-MIR contract instead of rediscovering ABI policy
locally.

## Core Rule

This plan may define and document AArch64 target-MIR call, return, and frame
responsibilities, but it must not emit or select final `bl`, `blr`, `ret`,
stack adjustment, save/restore, prologue, epilogue, or assembly forms.

## Read First

- `ideas/open/214_aarch64_aapcs64_call_return_frame_mir_contract.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`
- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/abi/abi.hpp`
- `src/backend/mir/aarch64/abi/abi.cpp`
- `src/backend/mir/aarch64/codegen/calls.md`
- `src/backend/mir/aarch64/codegen/returns.md`
- `src/backend/mir/aarch64/codegen/prologue.md`
- `src/backend/mir/aarch64/codegen/variadic.md`

## Current Targets

- a new or existing AArch64 contract document under `src/backend/mir/aarch64/`
  for AAPCS64 call, return, and frame target-MIR responsibilities
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- AArch64 ABI/module/codegen markdown that currently contains call, return,
  frame, prologue, variadic, memory-return, indirect-call, or special-register
  assumptions
- live target-MIR record surfaces only if inspection proves the contract cannot
  name existing prepared carriers accurately without narrow record additions
- focused docs or compile proof selected by the supervisor

## Non-Goals

- Do not emit or select final call, indirect-call, return, prologue, epilogue,
  stack-adjust, save/restore, or assembly instructions.
- Do not implement concrete frame layout, stack offsets, or outgoing call area
  allocation.
- Do not implement full variadic, aggregate return, exception, TLS, PLT/GOT,
  linker, or relocation behavior beyond explicit accepted deferral.
- Do not rewrite shared preparation algorithms.
- Do not use legacy assembly-text emitter behavior as target-MIR authority.
- Do not treat `x8`, `x16`, `x17`, `x29`, or `x30` as ordinary allocatable
  registers.

## Working Model

The accepted handoff should be:

```text
PreparedBirModule / prepared ABI, allocation, and frame facts
  -> AArch64 target-MIR call, return, and frame-boundary records/contracts
  -> later AArch64 machine instruction nodes
  -> optional printer / encoder / object output
```

The contract must say which prepared carriers are authoritative for argument
bindings, result bindings, memory-return ownership, caller-saved clobbers,
callee-save obligations, stack arguments, outgoing call area facts, frame
alignment, and reserved scratch policy.

When a required shared prepared fact is missing, the route must open a separate
`ideas/open/` gap instead of patching around the absence in AArch64.

## Execution Rules

- Start with inspection and contract boundaries; do not begin instruction
  selection under this plan.
- Prefer markdown contract and ledger updates unless a live record surface is
  too weak to express the accepted contract.
- Keep special-register roles explicit: `x8` for indirect result location
  ownership, `x16`/`x17` for IP/PLT/veneer-sensitive roles, `x29` for frame
  pointer policy, and `x30` for link-register return control.
- Treat variadic, aggregate, PLT/GOT, TLS, exception, and linker-sensitive
  behavior as accepted minimum contract or explicit deferral.
- Keep assembly text out of the semantic handoff.
- Each code-changing step needs fresh build or compile proof. Docs-only steps
  need stale-claim search plus `git diff --check`.

## Steps

### Step 1: Inspect Existing Call Return Frame Assumptions

Goal: identify the current AArch64 call, return, frame, and prologue authority
surfaces before writing the contract.

Primary target:
`src/backend/mir/aarch64/` markdown plus `module` and `abi` record surfaces.

Actions:

- Inspect `BIR_PREPARED_GAP_LEDGER.md`, AArch64 ABI docs/code, target-MIR
  module records, and codegen docs for direct call, indirect call, argument,
  return, memory-return, caller-saved, callee-save, stack-argument, outgoing
  call area, prologue, epilogue, and variadic assumptions.
- Identify any places that still treat old text-emitter behavior as authority.
- Identify which prepared allocation and ABI carriers are already sufficient
  after ideas 212 and 213.
- Record missing shared prepared facts as candidate separate ideas, not as
  AArch64-local workarounds.

Completion check:
`todo.md` records the concrete contract targets, stale assumptions, sufficient
prepared carriers, and missing-carrier candidates without implementation
edits.

### Step 2: Draft The AAPCS64 Target-MIR Contract

Goal: add the accepted target-MIR contract for AArch64 calls, returns, and
frame obligations.

Primary target:
a contract document under `src/backend/mir/aarch64/`.

Actions:

- Define direct-call and indirect-call target-MIR responsibilities.
- Define argument binding and return binding responsibilities over prepared
  carriers.
- Define memory-return handling and `x8` ownership.
- Define `x16`/`x17` indirect-call, PLT, veneer, and linker-sensitive roles.
- Define caller-saved clobber records and callee-save/frame obligations.
- Define stack-argument, stack-alignment, and outgoing call area facts.
- Define return-control and `x30` link-register responsibilities.
- Define `x29` frame-pointer policy at the target-MIR contract level.
- Define the minimum accepted variadic behavior or explicit deferral.

Completion check:
The contract can be read by a later instruction-selection executor without
needing to infer AAPCS64 policy from legacy codegen or assembly text.

### Step 3: Reconcile Ledgers And Markdown Consumers

Goal: make existing AArch64 docs point at the accepted contract and expose
gaps to later lifecycle work.

Primary target:
`src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md` and relevant
`src/backend/mir/aarch64/codegen/*.md`.

Actions:

- Update the gap ledger so call, return, frame, prologue, epilogue, memory
  return, indirect call, and variadic ownership is explicit.
- Replace stale language that lets feature slices locally choose argument
  registers, result registers, scratch registers, stack arguments, save/restore
  behavior, or frame layout policy.
- Keep machine-node selection, final instruction selection, assembler,
  encoder, object, and linker work assigned to later ideas.
- Create separate `ideas/open/` gap ideas only for missing carriers that block
  the contract from being accurate.

Completion check:
Markdown consumers consistently reference the accepted contract and do not
preserve hidden call/return/frame policy elsewhere.

### Step 4: Align Narrow Record Surface If Required

Goal: make minimal target-MIR record adjustments only if the contract cannot be
represented over existing records and prepared facts.

Primary target:
`src/backend/mir/aarch64/module/module.hpp` and
`src/backend/mir/aarch64/module/module.cpp`, only if inspection justifies code
changes.

Actions:

- Add narrow record fields or enums for contract facts that already exist in
  prepared data but lack a target-MIR representation.
- Preserve provenance for prepared ABI bindings, allocation facts, call sites,
  return sites, special-register authority, and frame obligations.
- Fail closed or split a new open idea when the source fact does not exist.
- Do not select concrete final instructions or compute concrete frame layout.

Completion check:
Any required record change is narrow, provenance-preserving, covered by focused
proof, and does not encode instruction-selection policy.

### Step 5: Add Focused Proof

Goal: prove the accepted contract and any narrow record changes are observable
without assembly text as the internal authority.

Primary target:
the repo-native AArch64 docs, module, or backend test surface selected by the
supervisor.

Actions:

- For docs-only completion, run stale-claim searches for old local ABI policy
  and `git diff --check`.
- For code changes, run the build and narrow backend test command selected by
  the supervisor.
- Add or update focused structured assertions only when live record changes
  were needed.
- Escalate to broader validation if shared prepared carriers or multiple
  backend buckets changed.

Completion check:
Fresh proof supports the contract without expectation downgrades, assembly
parsing, or named-case-only shortcuts.

## Completion Criteria

- A committed AArch64 contract defines target-MIR call, return, and frame
  boundaries over prepared call, allocation, and frame facts.
- Direct calls, indirect calls, memory returns, return registers, stack
  arguments, caller/callee-save obligations, `x8`, `x16`/`x17`, `x29`, `x30`,
  and reserved MIR scratch are explicitly handled or deferred.
- Missing BIR/prepared carriers discovered during the work are split into
  separate open ideas instead of bypassed in AArch64.
- Later instruction-selection slices can consume the contract instead of
  rediscovering AAPCS64 policy locally.
- Proof is fresh and appropriate to the actual changed surface.
