# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 789 local scalar load authority handoff.

## Purpose

Resume the typed Raw-BIR import route at its next authorized local-operation
receiver row without repeating accepted alloca receipt.

## Core Rule

Use only the native fields published by
`docs/lir_local_operation_authority/handoff_to_734.md`; local display spelling,
formatted operands, printer output, and LLVM text are never semantic input.

## Historical Progress

Steps 1 through 7.26 are accepted, including the selected alloca receiver in
`2cce9da69`. Preserve `006d79aaf`, `7dc03f23a`, and `2cce9da69`; do not redo
their work.

## Current Scope

- receive exactly the selected direct non-array/non-VLA local-scalar
  `LirLoadOp` from its valid `result`, `type_str`, `ptr`, and checked
  `local_object_authority` fields;
- add the minimum target-independent Raw-BIR container/importer/reachable
  verifier and transactional positive/negative coverage;
- require native result admission, pointer-definition equality,
  current-function valid/live authority, and pointee/load-type equality.

## Non-Goals

- all other direct/access/array/VLA loads, stores, GEPs, VLA lifetime, and
  named/local-temporary variants;
- memory/va, aggregate/vector, body parameters, module/type/global/metadata,
  CFG/PHI, target lowering, MIR, emission, and every later family;
- presentation-derived recovery or repeating Step 7.26.

## Ordered Steps

### Step 7.27 - Receive the selected direct local-scalar LirLoadOp authority

Goal: transactionally import the one 789-authorized load row into a typed
Raw-BIR destination.

Actions:

- map only the documented result, type, pointer definition, object/owner,
  pointer/pointee type, and liveness facts;
- validate all producer contract fields before publication and reject malformed
  rows transactionally;
- add nearby positive and negative receiver coverage, then run a fresh build
  and narrow proof before supervisor-selected broader acceptance proof.

Completion check: exactly this selected load row imports and verifies without
presentation recovery; all excluded local and later families remain fail
closed.
