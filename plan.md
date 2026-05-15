# Prepared I128 Helper Marshaling ABI Binding Runbook

Status: Active
Source Idea: ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md
Activated from: ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md
Supersedes active runbook: ideas/open/236_aarch64_i128_pair_lowering.md is parked on a helper-marshaling ABI-binding blocker.

## Purpose

Provide prepared/shared marshaling and ABI register-binding facts required
before AArch64 i128 div/rem helper-boundary records can print executable
terminal helper calls.

## Goal

Make low/high lane-to-ABI-register bindings, result bindings, helper-call
marshaling moves, clobber/live-value preservation, and selected-call operand
ownership explicit enough for idea 236 to consume without hard-coded helper
register assumptions.

## Core Rule

This runbook prepares helper marshaling authority only. It must not print
helper calls by hard-coding `x0`/`x1`, inferring adjacent registers, lowering
i128 as scalar i64, or synthesizing helper operands from opcodes/rendered text.

## Read First

- `ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md`
- `ideas/open/236_aarch64_i128_pair_lowering.md`
- `ideas/closed/248_prepared_i128_runtime_helper_authority.md`
- prepared i128 carrier, runtime-helper, ABI, call-clobber, and value-home
  facts
- focused backend tests under `tests/backend/mir/`

## Current Targets

- ABI argument register bindings for low/high i128 helper lanes.
- Direct-result register bindings for supported i128 div/rem helpers.
- Structured carrier-lane to helper-register marshaling facts.
- Helper call-clobber, live-value preservation, and resource facts.
- Selected-call operand ownership for terminal `bl <callee>` output.
- Fail-closed diagnostics for incomplete marshaling authority.

## Non-Goals

- Do not add float/i128 conversion helper mapping.
- Do not add memory-return helper-family support without a separate source idea.
- Do not implement generic call lowering, retained-call rewrites, binary128,
  atomics, intrinsics, inline asm, callee-save placement, or preserved-value
  extent work.
- Do not hard-code helper ABI registers in AArch64 printer or dispatch code.
- Do not weaken unsupported expectations to claim helper-call printing.

## Working Model

Idea 236 has selected `I128RuntimeHelperBoundaryRecord` values for supported
div/rem helpers, but terminal helper-call printing is still fail-closed because
helper marshaling and ABI register bindings are not structured. This runbook
adds the missing producer-owned facts so idea 236 can resume and print helper
boundaries from selected records.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Start by inspecting current selected helper-boundary records and prepared
  helper/call ABI facts.
- Add prepared/shared carriers where helper ABI binding and marshaling policy
  are already owned.
- Preserve fail-closed diagnostics for incomplete bindings, wrong carrier
  shapes, memory-return helpers, and unsupported helper families.
- Treat hard-coded ABI registers, rendered-name recovery, scalar-i64
  shortcuts, and expectation-only changes as route drift.
- For code-changing packets, prove with a build plus the supervisor-chosen
  focused prepared/i128 backend subset. Escalate to broader backend validation
  after shared ABI, call, or printer-visible facts change.

## Ordered Steps

### Step 1: Inspect Helper Marshaling And ABI Binding Gap

Goal: identify the exact facts missing between selected div/rem helper-boundary
records and terminal call printing.

Actions:

- Trace selected `I128RuntimeHelperBoundaryRecord` div/rem records into the
  current printer fail-closed diagnostic.
- Identify required ABI argument/result register bindings for low/high lanes.
- Identify required carrier-lane to ABI-register move/marshaling facts.
- Identify call-clobber, live-value preservation, and selected-call operand
  ownership facts needed for `bl <callee>` output.
- Record the first implementation packet target and proof subset in `todo.md`.

Completion check:

- `todo.md` names the exact producer-owned marshaling or ABI-binding carrier
  to implement first.

### Step 2: Add Low/High ABI Argument And Result Bindings

Goal: expose structured helper ABI register bindings for supported i128
div/rem helper arguments and direct results.

Actions:

- Define low/high argument register-binding facts for each supported div/rem
  helper operand.
- Define direct low/high result register-binding facts for supported div/rem
  helper results.
- Preserve lane order, lane width, register bank, helper kind, and callee
  identity as structured facts.
- Add diagnostics and focused coverage for incomplete binding state.

Completion check:

- Supported div/rem helper boundaries expose complete ABI argument and result
  register bindings for selected consumers.

### Step 3: Add Structured Helper Marshaling Facts

Goal: describe moves between prepared carrier lanes and helper ABI registers
without target-local register inference.

Actions:

- Define marshaling records from prepared source carrier lanes to helper ABI
  argument registers.
- Define result unmarshal records from helper ABI result registers to prepared
  result carrier lanes.
- Preserve value identity, lane identity, register bank, and move direction.
- Keep wrong carrier shapes and unsupported memory-return helpers fail-closed.

Completion check:

- Selected consumers can see every required carrier-to-helper and
  helper-to-carrier move as structured facts.

### Step 4: Add Call-Clobber And Live Preservation Authority

Goal: make helper-call clobber/resource and live-value preservation facts
explicit enough for terminal helper-call emission.

Actions:

- Connect helper clobber policy to the selected helper boundary.
- Preserve live carrier lanes that must survive the helper call.
- Expose selected-call operand ownership for terminal `bl <callee>` output.
- Add focused coverage for complete and incomplete helper-call policy facts.

Completion check:

- Helper-boundary consumers can emit or reject terminal calls from structured
  clobber/resource and selected-call ownership facts.

### Step 5: Validate And Hand Back To I128 Pair Lowering

Goal: prove the marshaling/ABI-binding prerequisite and hand back to idea 236.

Actions:

- Run the supervisor-chosen build and focused prepared/i128 backend subset.
- Escalate to broader backend validation if shared ABI, call, or
  printer-visible facts changed beyond one carrier.
- Summarize available div/rem helper ABI bindings, marshaling facts, and
  remaining unsupported helper shapes in `todo.md`.
- Ask the supervisor to route plan-owner to reactivate idea 236 only if helper
  call printing can consume these facts directly.

Completion check:

- The prerequisite facts are structurally present, incomplete facts still fail
  closed, and `todo.md` names whether idea 236 can resume helper-call printer
  consumption.
