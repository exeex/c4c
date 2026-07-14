# LIR SSA-Based Indexed-GEP Pointer-Result Authority

Status: Open (active blocker for
`ideas/open/764_lir_production_computed_goto_addr_value_publication.md`)
Type: bounded LIR GEP contract and production-authority repair
Predecessor: 764 Step 1, interrupted before downstream carrier publication

## Goal

Make a production SSA-based indexed GEP publish a verifier-valid,
current-function pointer-result `LirValueId`, so its direct caller can retain
the structured result rather than receiving only a rendered string.

## Why This Exists

764 Step 1 reproduced five computed-goto consumers that all fail only because
`LirIndirectBrOp.addr_value` is absent. Its statement seam already copies
`addr.value_id()`, but the address comes from
`src/codegen/lir/hir_to_lir/lvalue.cpp::emit_indexed_gep`, which emits a raw
result and returns string-only. The already accepted 765 handoff made the RHS
index authoritative; the remaining GEP base is SSA-based, while the current
verifier contract only authorizes GEP results with a global `LinkNameId` base.

The rejected stmt-only pointer-bitcast bridge is not a substitute: it fails
`verify_cast_op_authority` because authoritative casts require integer
endpoints. This is the first evidenced owner of the missing address-result
identity, upstream of 764's carrier publication.

## In Scope

- Define the minimal verifier-valid authority contract for a current-function
  pointer result of an indexed GEP whose base is an SSA value, retaining the
  existing required structured base/index/result facts.
- Update the direct production `emit_indexed_gep` path to allocate and retain
  that pointer-result `LirValueId` under the new contract rather than returning
  a string-only result.
- Add direct nearby positive and malformed-authority coverage for this
  SSA-based indexed-GEP result path, including fail-closed rejection for
  missing, invalid, foreign, non-pointer, or otherwise unauthorized inputs as
  the final contract requires.
- Hand off the typed GEP result and exact proof to 764 Step 1 only.

## Out Of Scope

- Publishing `LirIndirectBrOp.addr_value`, changing `IndirBrStmt`, or proving
  the five computed-goto consumers; those remain 764 Step 1 after this blocker.
- Raw-BIR containers/importer work or re-executing 734 Step 7.24.
- Verifier weakening, partial/raw authority, display/printer/LLVM/label/testcase
  text recovery, or a synthetic alloca/load, phi, select, cast, or other
  statement-seam identity bridge.
- General pointer, rvalue, CFG, PHI, local/object, memory/va,
  aggregate/vector, call, target-lowering, MIR, or emission-family redesign.

## Acceptance Criteria

- A current-function SSA-based indexed GEP with valid required structured
  operands has a verifier-valid pointer-result `LirValueId`, and the direct
  `emit_indexed_gep` production path retains it structurally.
- Direct nearby coverage proves the positive production path and preserves
  fail-closed malformed-input rejection; no GEP result is authorized from
  raw, partial, foreign, invalid, or text-derived facts.
- A fresh build and the focused contract/producer proof pass, with the handoff
  naming the result field/ID, producer seam, and exact return: resume 764
  Step 1 to publish that verified GEP ID into `LirIndirectBrOp.addr_value` and
  rerun its five consumers.

## Reviewer Reject Signals

- Reject a verifier relaxation, global-base-only bypass, partial/raw operand
  admission, or a patch that still leaves the production SSA GEP result
  string-only behind a renamed helper.
- Reject text parsing or recovery from rendered operands, labels, printers,
  LLVM, or testcase identity, including testcase-shaped branching.
- Reject a cast or synthetic alloca/load, phi, or select used solely to bridge
  the missing result identity instead of repairing the GEP contract/producer.
- Reject any `LirIndirectBrOp`, Raw-BIR/importer, 734 Step 7.24, or broad
  adjacent authority-family work claimed as progress for this bounded GEP
  result contract.
