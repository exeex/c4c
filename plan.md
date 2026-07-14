# Production Computed-Goto Address Authority Runbook

Status: Active
Source Idea: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Resumed from: concluded 765 Step 1 (`1e24e2081`), which now carries the valid
`insn.f1.offset` member/bitfield RHS `LirValueId` required for the address GEP.

## Purpose

Complete only the downstream production computed-goto carrier repair revealed
by the accepted 765 producer handoff. The rejected full-suite candidate shows
one consumer failure family, not acceptable baseline debt.

## Goal

For the production computed-goto address route, publish the verified
current-function pointer `LirValueId` into `LirIndirectBrOp.addr_value` so the
five affected consumers no longer fail at that carrier check.

## Core Rule

`addr_value` is semantic authority and can be published only from the valid,
current-function pointer address result. `addr`, labels, printer output,
rendered LLVM, and testcase names are never authority sources.

## Read First

- `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
- `ideas/closed/765_lir_member_bitfield_rvalue_value_identity_publication.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` resumption record
- the computed-goto `IndirBrStmt` producer, `emit_indexed_gep`, and existing
  `LirIndirectBrOp` verifier checks

## Landed Progress

- 765 Step 1 is accepted in `1e24e2081`: its focused
  `frontend_lir_call_type_ref` proof passed, matching subset guard passed, and
  broad backend guard passed.
- The previous full baseline (`c8a205218`) was 3034/3034. The candidate after
  `1e24e2081` has five failures: `comp-goto-1`, `20040302-1`, `20041214-1`,
  `920501-4`, and `920501-5`. Each stops at the same missing
  `LirIndirectBrOp.addr_value` current-function-pointer authority check.

## Non-Goals

- no Raw-BIR/importer changes or re-execution of 734 Step 7.24
- no GEP verifier relaxation, partial/raw-index publication, display-text
  recovery, failure exclusions, expectation downgrades, or baseline exceptions
- no general rvalue, CFG, PHI, local/object, memory/va, aggregate/vector,
  target-lowering, MIR, or emission redesign

## Execution Rules

1. Repair the common downstream carrier seam, not a named consumer.
2. Preserve fail-closed rejection for missing, invalid, foreign, non-pointer,
   display-mismatched, and partial/raw authority.
3. Prove the repaired consumer family before asking the supervisor to run a
   future full baseline candidate. No candidate may be accepted if it expands
   baseline failures.

## Ordered Steps

### Step 1 - Publish and prove production computed-goto address carrier authority

Goal: use the now-valid production address GEP identity to populate and verify
`LirIndirectBrOp.addr_value`, eliminating the same authority failure across
the five affected consumers.

Primary targets:

- the immediate computed-goto address-to-`LirIndirectBrOp` publication seam
- nearby verifier and focused production-path coverage

Actions:

- reproduce the five affected tests and confirm their shared carrier failure
  before changing code
- trace from the authoritative address GEP to the `IndirBrStmt` publication
  point; minimally preserve the valid pointer `LirValueId` into `addr_value`
- retain the existing malformed-authority checks and add/extend focused
  production-path coverage without testcase-specific branching
- after the narrow proof, rerun all five affected tests; the full candidate is
  a later supervisor gate and must demonstrate no new baseline failures before
  acceptance
- record the typed-field handoff and return only to 734 after its accepted
  Step 7.24; do not repeat the receiver packet

Completion check:

- a fresh build, focused positive/malformed proof, and the five affected
  consumers show no `LirIndirectBrOp.addr_value` missing-authority failure;
  a future supervisor full-suite candidate has no new baseline failures before
  it can be accepted.
