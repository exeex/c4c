# Typed Direct Label-Address Constant GEP Contract Runbook

Status: Active
Source Idea: ideas/open/773_lir_gep_direct_label_address_constant_contract.md
Resumed from: preserved 773 Step 2 after completed blocker 774

## Purpose

Enable only a verified current-function direct label-address constant to serve
as a typed `LirGepOp.ptr`, so the parent production seam can later retain its
authority without text recovery or SSA fabrication.

## Goal

Transition verifier, printer, and backend/lowering contracts for the narrow
`DirectConstant(LirValueId)` direct-label-address GEP base form.

## Core Rule

Direct constants remain function-owned typed authority. Admit them only after
form-specific validation proves current-function direct-label-address identity;
neither display text nor synthetic SSA is authority.

## Accepted Progress

- Step 1 is accepted in `a4415f99c` (`lir: verify direct label constants as
  GEP bases`): verifier admission plus nearby interface coverage for the typed
  `DirectConstant(LirValueId)` GEP base. Fresh build and
  `^backend_lir_to_bir_interface$` proof passed.
- The separately scoped Raw-BIR representation blocker completed in
  `c64b78c48` and `97121c359`. Its fresh build and
  `^backend_lir_to_bir_interface$` proof passed; matched `^backend_`
  before/after guards passed 5/5. This is Step 2 input, not Step 2 completion.

## Read First

- `ideas/open/773_lir_gep_direct_label_address_constant_contract.md`
- `ideas/closed/774_raw_bir_gep_function_label_address_base.md`
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
3. Use 774's typed Raw-BIR authority directly; no text recovery, global
   coercion, or fabricated SSA/global is permitted.
4. Add nearby positive and malformed contract coverage; testcase names and
   rendered text are never selectors or authority sources.
5. Do not refresh or accept the rejected 3037/1 baseline candidate: 772 must
   first repair structured forwarding and make `pr70460` pass.

## Ordered Steps

### Step 1 - Specify and verify the typed direct-label-address GEP base

Status: Complete (`a4415f99c`).

Goal: make `LirGepOp.ptr` admit exactly the valid function-owned direct
label-address `DirectConstant(LirValueId)` form.

Completion check: verifier tests show the exact allowed form passes and the
relevant malformed forms fail closed. Accepted proof: fresh build plus
`^backend_lir_to_bir_interface$` passed.

### Step 2 - Carry the validated form through printer and backend/lowering

Goal: represent and lower the verified typed direct constant as a GEP base.

Primary targets:

- LIR GEP printer operand handling
- backend/lowering GEP base dispatch
- focused printer/lowering contract tests

Actions:

- add the exact validated direct-label-address branch where SSA/global is
  currently assumed
- use typed direct-constant resolution and 774's structured Raw-BIR authority,
  never display text or a fabricated SSA/global value
- retain fail-closed behavior for unverified or unsupported constants
- prove printer and lowering agree with verifier admission, with nearby
  positive and malformed coverage
- do not edit 772 forwarding, pr70460, or baseline artifacts

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
- state that 772 must repair structured `emit_indexed_gep` forwarding and make
  `llvm_gcc_c_torture_src_pr70460_c` pass before any new baseline evaluation

Completion check:

- fresh build and narrow proof pass, acceptance evidence is ready for the
  supervisor, and the parent return action is unambiguous.
