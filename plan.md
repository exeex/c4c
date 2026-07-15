# LIR-To-New-BIR Local Store Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 790 local scalar store authority handoff.

## Purpose

Receive the one producer-authorized local scalar declaration store in typed
Raw-BIR without repeating accepted alloca or load receipt.

## Core Rule

Use only the native fields in
`docs/lir_local_operation_authority/handoff_to_734.md`. Local spelling,
formatted operands, printer output, LLVM text, and testcase identity are never
semantic input.

## Historical Progress

Steps 1 through 7.27 are accepted, including `006d79aaf`, `7dc03f23a`,
`2cce9da69`, and `eabf7a3b8`. Closed 790 published the selected store authority
in `727949c9c`; do not redo producer work.

## Current Scope

- receive exactly the direct non-array/non-VLA integer local scalar declaration
  `LirStoreOp` from its native immediate value, type, pointer definition, and
  checked local-object owner/type/liveness authority;
- add the minimum target-independent Raw-BIR destination/importer/reachable
  verifier and transactional positive/negative coverage;
- require selected-store admission, representable native immediate,
  pointer-definition equality, current-function valid/live authority, and
  pointee/store-type equality.

## Non-Goals

- assignment/SSA/pointer/aggregate/vector/array/VLA stores, every GEP and
  later local load, VLA lifetime, and named/local-temporary variants;
- memory/va, aggregate/vector, body parameters, module/type/global/metadata,
  CFG/PHI, target lowering, MIR, emission, and every later family;
- presentation-derived recovery or repeating accepted alloca/load receipt.

## Ordered Steps

### Step 7.28 - Receive the selected direct local-scalar LirStoreOp authority

Goal: transactionally import the one 790-authorized local store row into a
typed Raw-BIR destination.

Actions:

- map only native immediate, type, pointer definition, object/owner,
  pointer/pointee type, and liveness facts;
- validate the selected-store admission and all producer contract fields before
  publication, rejecting malformed rows transactionally;
- add nearby positive and negative receiver coverage, then run a fresh build
  and narrow proof before supervisor-selected broader acceptance proof.

Completion check: exactly this selected store imports and verifies without
presentation recovery; all excluded local and later families remain fail
closed.
