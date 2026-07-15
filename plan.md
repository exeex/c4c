# LIR-To-New-BIR VLA Stack-Save Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 792 VLA `LirStackSaveOp` authority handoff.

## Purpose

Receive exactly closed 792's selected VLA stack-save saved-stack-pointer row
in typed Raw-BIR without repeating accepted alloca, load, store, or local-array
GEP receipts.

## Core Rule

Use only the native fields in
`docs/lir_local_operation_authority/handoff_to_734.md`. Local spelling,
formatted operands, printer output, LLVM text, and testcase identity are never
semantic input.

## Historical Progress

Steps 1 through 7.29 are accepted, including receiver commits `006d79aaf`,
`7dc03f23a`, `2cce9da69`, `eabf7a3b8`, `f5cda70ee`, and `4ab2deb7e`.
Closed 792 published this exact stack-save authority in `900a42bfd` and
`89f4d7f85`; do not redo producer work.

## Current Scope

- receive exactly one selected VLA `LirStackSaveOp` saved-stack-pointer result
  with native valid current-function result, pointer definition, object/owner,
  pointer-type/pointee-type, and liveness authority;
- add the minimum target-independent Raw-BIR saved-stack-pointer destination,
  importer dispatch, reachable verifier, and transactional positive/negative
  coverage;
- require selected row admission and complete native authority coherence before
  publication, rejecting malformed rows transactionally.

## Non-Goals

- `LirStackRestoreOp`, dynamic VLA allocation, any second stack save, local
  load/store/GEP, named/local-temporary, or any other local row;
- memory/va, aggregate/vector, body parameters, module/type/global/metadata,
  CFG/PHI, target lowering, MIR, emission, and every later family;
- presentation-derived recovery or repeating accepted producer work.

## Ordered Steps

### Step 7.30 - Receive the selected VLA LirStackSaveOp authority

Goal: transactionally import closed 792's one saved-stack-pointer result into
a typed Raw-BIR destination.

Actions:

- map only the native result, pointer definition, object/owner,
  pointer-type/pointee-type, and liveness facts;
- validate selected-save admission and every producer contract field before
  publication, rejecting malformed or nonselected rows transactionally;
- add nearby positive and negative receiver coverage, then run a fresh build
  and narrow proof before supervisor-selected broader acceptance proof.

Completion check: exactly the selected VLA stack-save result imports and
verifies without presentation recovery; stack restore, dynamic VLA allocation,
and all excluded local and later families remain fail closed.
