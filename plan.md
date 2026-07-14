# Typed Direct Label-Address Constant GEP Contract Runbook

Status: Active
Source Idea: ideas/open/773_lir_gep_direct_label_address_constant_contract.md
Supersedes: active 772 Step 1 while its out-of-scope contract blocker runs

## Purpose

Enable only a verified current-function direct label-address constant to serve
as a typed `LirGepOp.ptr`, so the parent production seam can later retain its
authority without text recovery or SSA fabrication.

## Goal

Transition verifier, printer, and backend/lowering contracts for the narrow
`DirectConstant(LirValueId)` direct-label-address GEP base form.

## Core Rule

Direct constants remain function-owned typed authority. Admit them only after
the same form-specific validation proves current-function direct-label-address
identity; neither display text nor synthetic SSA is authority.

## Read First

- `ideas/open/773_lir_gep_direct_label_address_constant_contract.md`
- `ideas/open/772_lir_gep_pointer_authority_pr70460.md` (resumption record)
- existing direct-label-address constant verifier/printer/backend contracts
- existing `LirGepOp` verification, printing, and lowering paths

## Non-Goals

- no `pr70460` `emit_indexed_gep` forwarding-seam edit
- no generic GEP pointer-model redesign or unrelated pointer operand widening
- no synthetic SSA or text/RawText recovery
- no computed-goto carrier, Raw-BIR/importer, or 734 work

## Execution Rules

1. Preserve fail-closed rejection for all operand forms other than the exact
   validated current-function direct label-address constant.
2. Keep verifier, printer, and lowering behavior aligned; do not make one
   layer accept an operand another layer cannot represent or lower.
3. Add nearby positive and malformed contract coverage; testcase names and
   rendered text are never selectors or authority sources.
4. Build fresh and run narrow proof before reporting the parent handoff. The
   supervisor, not this runbook, evaluates a later full-suite candidate.

## Ordered Steps

### Step 1 - Specify and verify the typed direct-label-address GEP base

Goal: make `LirGepOp.ptr` admit exactly the valid function-owned direct
label-address `DirectConstant(LirValueId)` form.

Primary targets:

- `verify_authoritative_gep` and related pointer-operand verification
- `LirFunction::direct_label_address_constants` ownership/definition checks
- focused verifier positive and malformed tests

Actions:

- identify the shared and GEP-specific checks that currently exclude the
  direct constant
- add form-specific validation for ID validity, current-function ownership,
  direct-label-address identity, pointer suitability, and display obligations
- retain rejection of missing, foreign, arbitrary, non-pointer, and malformed
  direct constants
- add focused contract tests without editing the parent production seam

Completion check:

- verifier tests show the exact allowed form passes and the relevant malformed
  forms fail closed with the existing contract strength preserved.

### Step 2 - Carry the validated form through printer and backend/lowering

Goal: represent and lower the verified typed direct constant as a GEP base.

Primary targets:

- LIR GEP printer operand handling
- backend/lowering GEP base dispatch
- focused printer/lowering contract tests

Actions:

- add the exact validated direct-label-address branch where SSA/global is
  currently assumed
- use the typed direct-constant resolution path, never display text or a
  fabricated SSA value
- retain fail-closed behavior for unverified or unsupported constants
- prove the printer and lowering agree with verifier admission

Completion check:

- focused printer/lowering coverage exercises the allowed typed operand and
  rejects unsupported forms without changing the parent forwarding seam.

### Step 3 - Prove the bounded contract and hand it back to 772

Goal: deliver an accepted contract transition with a precise parent return.

Actions:

- run a fresh build and the selected focused verifier/printer/lowering subset
- inspect that tests cover positive and malformed boundaries rather than a
  single named case
- report changed contracts, exact proof, and the handoff to resume 772 Step 1

Completion check:

- fresh build and narrow proof pass, acceptance evidence is ready for the
  supervisor, and the parent return action is unambiguous.
