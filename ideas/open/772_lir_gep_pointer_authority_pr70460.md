# Production LIR GEP Pointer Authority for pr70460

Status: Open (active baseline-clearing blocker)
Type: bounded production LIR GEP pointer-authority repair
Predecessor: closed
`ideas/closed/764_lir_production_computed_goto_addr_value_publication.md`

## Goal

Repair the production path behind `pr70460` so each emitted `LirGepOp` has the
required non-empty, valid current-function pointer authority in `ptr`, without
weakening its verifier contract or deriving identity from rendered text.

## Why This Exists

The prior full-suite candidate is rejected, not accepted: it has five
failures. Commit `9680b15b9` accepted the independent four-case computed-goto
carrier repair and its five-consumer guard now passes 5/5. The remaining known
failure is `pr70460` stopping at `LirGepOp.ptr: must not be empty`. It is not
part of 764's `LirIndirectBrOp.addr_value` family, yet it prevents a future
full-suite candidate from clearing the rejected baseline.

## In Scope

- reproduce `pr70460` and trace the first production owner that emits or
  forwards an empty `LirGepOp.ptr`
- make only that proven GEP-pointer producer/publication seam retain a typed,
  valid current-function pointer `LirValueId`
- preserve the existing `LirGepOp.ptr` presence, validity, ownership, pointer
  suitability, and display-mirror verifier checks
- add nearby production-path positive and malformed-authority coverage, then
  prove the repaired case with a fresh build and a focused test subset
- provide a precise baseline handoff: the original five-failure candidate
  remains rejected until the supervisor runs and evaluates a new full-suite
  candidate

## Out Of Scope

- the accepted 764 computed-goto carrier repair, its five-case guard, or any
  rework of `LirIndirectBrOp.addr_value`
- Raw-BIR/importer work, 734 Step 7.24 receiver re-execution, or 734 lifecycle
  disposition before this baseline blocker is resolved
- reimplementation of accepted 765/766/767/768 producer contracts absent new
  first-bad-fact evidence
- verifier relaxation, raw/partial IDs, text/label/printer/LLVM recovery,
  testcase routing, expectation downgrade, failure exclusion, or baseline-log
  changes
- broad rvalue, GEP/table, CFG, PHI, local/object, memory/va,
  aggregate/vector, target-lowering, MIR, or backend redesign

## Acceptance Criteria

- `pr70460` no longer fails because `LirGepOp.ptr` is empty, and the repair is
  at the first evidenced production authority-loss seam rather than a
  testcase-specific bypass.
- The emitted GEP pointer is a valid current-function pointer `LirValueId`;
  nearby malformed cases still fail closed for missing, invalid, foreign,
  non-pointer, or display-mismatched authority.
- A fresh build plus focused production-path and neighboring authority proof
  pass without weakening contracts or changing baseline artifacts.
- The supervisor receives the exact producer seam, typed-field handoff,
  commit/proof references, and whether a fresh full-suite candidate clears or
  leaves additional independently scoped failures.

## Reviewer Reject Signals

- Reject any `ptr` value recovered from operand spelling, labels, rendered
  LLVM/printer output, or a `pr70460` testcase name.
- Reject a named-case-only branch, expectation downgrade, failure exclusion,
  baseline refresh/log edit, or verifier relaxation claimed as GEP capability
  progress.
- Reject synthetic `select`, `gep`, `bitcast`, alloca/load, PHI, or dummy
  instruction IDs that retain the empty production-pointer failure behind a
  new abstraction name.
- Reject Raw-BIR/importer or 734 receiver changes, computed-goto carrier
  rework, or a broad GEP/rvalue/table redesign without first-bad-fact evidence.
- Reject focused-only acceptance that does not preserve malformed fail-closed
  checks and a stated full-suite baseline disposition.

## Resumption Record

- Last accepted progress: Step 1 mapping only; no implementation or test
  change, proof acceptance, or code commit was made.
- Interrupted step: Step 1 — `Trace and repair the production GEP pointer
  authority loss`.
- First bad fact and blocker: `StmtEmitter::emit_indexed_gep(FnCtx&, const
  LirOperand&, ...)` at `src/codegen/lir/hir_to_lir/lvalue.cpp:958` receives
  the typed `DirectConstant(LirValueId)` for `&&lab0`, but sends `.str()`
  (empty) to the string overload, which emits `RawText(\"\")` as
  `LirGepOp.ptr`. Retaining that typed direct constant is blocked because the
  verifier, printer, and backend/lowering contracts only admit SSA/global GEP
  bases; synthetic SSA is invalid for a function-owned direct label-address
  constant. That contract work is explicitly outside this idea's scope.
- Blocker route: `ideas/open/773_lir_gep_direct_label_address_constant_contract.md`
  owns the narrowly scoped verifier/printer/backend transition for a verified
  current-function direct label-address constant as `LirGepOp.ptr`.
- Exact return point: after 773 is accepted, resume this idea at Step 1 by
  repairing the structured `emit_indexed_gep` direct-constant forwarding seam
  with the newly supported typed direct-constant authority. Do not repeat the
  authority mapping or change the blocker contracts here.
- Remaining action and handoff: add nearby direct-label-address production and
  malformed-authority coverage for the seam, then run a fresh build plus the
  focused `pr70460` proof and give the supervisor a full-suite candidate
  handoff.
- Proof references: the mapping run used a fresh build with `ctest --test-dir
  build --output-on-failure -R '^llvm_gcc_c_torture_src_pr70460_c$'`; it failed
  as expected at empty `LirGepOp.ptr`. Canonical baseline logs retain that
  baseline. Accepted implementation proof and commit references: none.

## Resumption Update: accepted 773 handoff

The separately scoped direct-label-address GEP contract blocker is closed as
capability complete at
`ideas/closed/773_lir_gep_direct_label_address_constant_contract.md`.

- Accepted blocker contract commits: `a4415f99c` (verifier), `c64b78c48` and
  `97121c359` (typed Raw-BIR prerequisite), and `0d0f0725b` (table-backed
  printer and typed lowering).
- Accepted blocker proof: the Step 2 fresh build passed; exact
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 5/5;
  matching before/after monotonic guard passed with the allowed
  non-decreasing count.
- Current return point: resume Step 1, `Trace and repair the production GEP
  pointer authority loss`, at the already evidenced
  `StmtEmitter::emit_indexed_gep(FnCtx&, const LirOperand&, ...)` forwarding
  seam. Forward the typed direct constant structurally through the now
  supported GEP base contract; do not repeat the mapping or alter 773's
  verifier/printer/lowering boundary.
- Remaining action: add nearby production and malformed-authority coverage,
  fresh-build and make `llvm_gcc_c_torture_src_pr70460_c` pass, then provide a
  supervisor-owned fresh full-suite candidate handoff. The rejected baseline
  stays rejected and must not be evaluated anew before this Step 1 result.
