# LIR-To-New-BIR Direct-Local VaStart Receiver Runbook

Status: Exhausted - Pending Plan-Owner Decision
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Activated from: docs/lir_memory_va_object_lifetime_authority/handoff_to_734.md

## Purpose

Resume idea 734 for exactly one accepted receiver row from closed idea 867:
selected direct-local `LirVaStartOp` destination `va_list` authority.

## Goal

Receive the handed-off native `LirVaStartOp.ap_authority` tuple into typed Raw
BIR without display-text recovery.

## Core Rule

Implement one Raw-BIR receiver packet only. The authority is the structured
LIR tuple named by the 867 handoff, never `ap_ptr` spelling, printer output,
LLVM text, intrinsic names, rendered names, prepared-BIR helper state, Raw-BIR
importer guesses, or testcase identity.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `docs/lir_memory_va_object_lifetime_authority/handoff_to_734.md`
- Existing Raw-BIR container, importer, and verifier code for received memory,
  local-object, VLA, and VA-related rows.
- `src/codegen/lir/verify.cpp` only as the accepted LIR verifier boundary;
  do not edit producer authority in this receiver packet.

## Current Scope

- Receive only selected direct-local `LirVaStartOp` destination `va_list`
  pointer authority.
- Preserve these handed-off fields: source `ap_ptr.value_id()`,
  `pointer_definition`, `object`, `owner`, `pointer_type`, `pointee_type`, and
  `live`.
- Require `ap_ptr.value_id()` to equal
  `ap_authority.local_pointer.pointer_definition`.
- Add only the minimum typed Raw-BIR destination, importer dispatch,
  reachable verifier path, and transactional positive/negative coverage for
  this row.

## Non-Goals

- No LIR producer/schema/verifier edits for the selected authority.
- No `va_end`, `va_copy`, `va_arg`, memcpy, memset, local-object, VLA,
  prepared-BIR helper-home, target backend lowering, or MIR work.
- No aggregate/vector, function-body parameter, module/type/global/metadata,
  CFG/PHI, residual instruction/terminator, inline-assembly, or generic
  residual sweep.
- No receipt from operand spelling, printer output, LLVM text, intrinsic
  names, rendered names, `monostate`, compatibility mirrors, or testcase
  identity.
- No repeat of accepted idea 734 Steps 1 through 7.51.

## Working Model

Closed idea 867 proves the producer/verifier side only. This runbook consumes
that already accepted authority at the Raw-BIR boundary and leaves all other
memory/VA/object/lifetime rows fail closed until their own handoff is accepted.

## Execution Rules

- Keep the implementation packet bounded to Step 7.52.
- Reject malformed selected `va_start` before any partial Raw-BIR publication.
- Preserve the existing LIR verifier boundary and treat its checks as
  prerequisite authority, not receiver implementation.
- Add same-feature positive and malformed receiver coverage near the existing
  Raw-BIR importer/interface tests.
- Run a fresh build plus focused backend receiver proof. Escalate to broader
  backend proof if shared Raw-BIR container, verifier, or importer helpers are
  touched.

## Ordered Steps

### Step 7.52 - Receive selected direct-local LirVaStartOp destination va_list authority

Status: Complete

Goal: consume the 867 `LirVaStartOp.ap_authority` handoff in typed Raw BIR.

Primary targets:
- Raw-BIR typed destination, builder/view, importer dispatch, reachable
  verifier, and nearby backend receiver tests for the selected `va_start` row.

Actions:
- Add the minimum typed Raw-BIR representation for the selected direct-local
  `va_start` destination pointer authority.
- Import only selected `LirVaStartOp` rows whose `ap_authority` is present and
  whose `ap_ptr.value_id()` equals
  `ap_authority.local_pointer.pointer_definition`.
- Verify current-function ownership, live local object authority, pointer and
  pointee type agreement, canonical local pointer fact agreement, and
  fail-closed rejection for selected malformed rows.
- Reject authority fields on unselected `va_start` rows.
- Add transactional positive and negative receiver coverage for missing or
  invalid `ap_authority`, invalid or mismatched `ap_ptr` value id, foreign
  owner, dead local object, pointer or pointee type mismatch, canonical local
  pointer disagreement, and unselected `va_start` authority fields.

Completion check:
- The selected direct-local `LirVaStartOp` row imports into verified Raw BIR
  using only the handed-off structured tuple.
- Every malformed selected or unselected boundary above rejects before partial
  publication.
- Fresh build, focused backend receiver proof, `git diff --check`, and any
  supervisor-selected broader backend proof pass.

Result:
- Added `VaStartAuthority` Raw-BIR opcode/payload/spec/view support.
- Added importer validation and dispatch for only selected direct-local
  `LirVaStartOp` rows whose `ap_ptr.value_id()` matches the authority pointer
  definition.
- Added FoundationVerifier checks requiring a matching live alloca authority
  with exact owner, object, pointer type, pointee type, and liveness.
- Added focused positive and malformed receiver coverage in
  `backend_lir_to_bir_interface`.
- Proof passed with the focused receiver/producer command, `git diff --check`,
  and monotonic regression guard.
