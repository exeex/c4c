# Production LIR GEP Pointer Authority Runbook for pr70460

Status: Active
Source Idea: ideas/open/772_lir_gep_pointer_authority_pr70460.md
Resumed from: closed 773 direct-label-address GEP contract blocker

## Purpose

Repair only the remaining known `pr70460` production GEP-pointer authority
failure that prevents a future full-suite candidate from clearing the rejected
baseline.

## Goal

Publish the verified current-function direct-label-address constant into
`LirGepOp.ptr` at the first evidenced production authority-loss seam.

## Core Rule

`ptr` is semantic authority. Forward the verified typed direct constant only
through structured operands; display operands, labels, printer output,
rendered LLVM, and testcase names are never authority sources.

## Read First

- `ideas/open/772_lir_gep_pointer_authority_pr70460.md`
- `ideas/closed/773_lir_gep_direct_label_address_constant_contract.md`
- the `pr70460` production lowering route and existing `LirGepOp` verifier
  checks

## Landed Prerequisites

- 764 Step 1 (`9680b15b9`) is closed and accepted: its five computed-goto
  consumers pass 5/5 and are not this route's target.
- 773 is closed and accepted: `a4415f99c`, `c64b78c48`, `97121c359`, and
  `0d0f0725b` authorize this exact typed direct-label-address GEP base through
  verifier, printer, and lowering. Do not reopen those boundaries here.

## Non-Goals

- no Raw-BIR/importer changes or re-execution of 734 Step 7.24
- no 764 computed-goto carrier rework, verifier/printer/lowering contract
  changes, partial/raw authority, display-text recovery, failure exclusion,
  expectation downgrade, or baseline exception
- no broad rvalue, table, CFG, PHI, local/object, memory/va,
  aggregate/vector, target-lowering, MIR, or emission-family redesign

## Execution Rules

1. Start at the already evidenced `StmtEmitter::emit_indexed_gep` structured
   direct-constant forwarding seam; do not repeat the mapping or alter 773.
2. Repair only that seam by retaining the verified typed direct constant into
   `LirGepOp.ptr`, preserving all fail-closed checks.
3. Add nearby production-path and malformed-authority coverage; do not turn
   the failure into an allowed or text-derived case.
4. Fresh-build and make `llvm_gcc_c_torture_src_pr70460_c` pass before any
   new baseline evaluation. Focused proof is not full-baseline acceptance.

## Ordered Steps

### Step 1 - Trace and repair the production GEP pointer authority loss

Goal: minimally repair the first evidenced forwarding seam that leaves
`LirGepOp.ptr` empty for `pr70460`.

Primary targets:

- `StmtEmitter::emit_indexed_gep(FnCtx&, const LirOperand&, ...)`
- nearby `LirGepOp` production-path and malformed-authority coverage

Actions:

- forward the already validated typed direct constant structurally rather than
  calling the string overload that creates `RawText(\"\")`
- preserve current-function ownership and the 773 verifier/printer/lowering
  contract; do not fabricate an SSA/global value
- add or extend nearby production-path and malformed-authority coverage
- fresh-build, prove `llvm_gcc_c_torture_src_pr70460_c`, and report the
  producer seam, typed field, proof, and next baseline action to supervisor

Completion check:

- `pr70460` no longer fails at empty `LirGepOp.ptr`; fresh build and focused
  positive/malformed proof pass with contracts intact; the supervisor has an
  exact fresh-full-suite candidate handoff.
