# LIR-To-New-BIR Local Array GEP Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 791 static-local-array GEP authority handoff.

## Purpose

Receive the one producer-authorized direct static-local-array GEP in typed
Raw-BIR without repeating accepted alloca, local-load, or declaration-store
receipt.

## Core Rule

Use only the native fields in
`docs/lir_local_operation_authority/handoff_to_734.md`. Local spelling,
formatted operands, printer output, LLVM text, and testcase identity are never
semantic input.

## Historical Progress

Steps 1 through 7.28 are accepted, including `006d79aaf`, `7dc03f23a`,
`2cce9da69`, `eabf7a3b8`, and `f5cda70ee`. Closed 791 published this exact
GEP authority in `ea579c648`; do not redo producer work.

## Current Scope

- receive exactly the direct static-local-array `LirGepOp` with native valid
  result ID, element type, SSA base pointer, one i64 immediate index, and
  checked current-function local-object owner/type/liveness authority;
- add the minimum target-independent Raw-BIR destination/importer/reachable
  verifier and transactional positive/negative coverage;
- require selected-GEP admission, result/base validity and equality checks,
  exact one native i64 immediate index, element/pointee type coherence, and
  valid/live current-function authority.

## Non-Goals

- SSA-indexed, local-temporary, aggregate-member, VLA, or any other GEP;
  every store, later load, stack lifetime, and nonselected local row;
- memory/va, aggregate/vector, body parameters, module/type/global/metadata,
  CFG/PHI, target lowering, MIR, emission, and every later family;
- presentation-derived recovery or repeating accepted alloca/load/store work.

## Ordered Steps

### Step 7.29 - Receive the selected direct static-local-array LirGepOp authority

Goal: transactionally import the one 791-authorized local-array GEP into a
typed Raw-BIR destination.

Actions:

- map only native result, element type, pointer definition, immediate index,
  and object/owner/pointer/pointee/liveness facts;
- validate selected-GEP admission and all producer contract fields before
  publication, rejecting malformed rows transactionally;
- add nearby positive and negative receiver coverage, then run a fresh build
  and narrow proof before supervisor-selected broader acceptance proof.

Completion check: exactly this selected GEP imports and verifies without
presentation recovery; all excluded local and later families remain fail
closed.
