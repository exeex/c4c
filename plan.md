# Production LIR GEP Pointer Authority Runbook for pr70460

Status: Active
Source Idea: ideas/open/772_lir_gep_pointer_authority_pr70460.md
Activated from: closed 764 carrier repair; the rejected baseline remains
active until this independently scoped failure receives accepted proof.

## Purpose

Repair only the remaining known `pr70460` production GEP-pointer authority
failure that prevents a future full-suite candidate from clearing the rejected
baseline.

## Goal

Publish a verified current-function pointer `LirValueId` into `LirGepOp.ptr`
at its first evidenced production authority-loss seam.

## Core Rule

`ptr` is semantic authority. Publish it only from a verified current-function
pointer result; display operands, labels, printer output, rendered LLVM, and
testcase names are never authority sources.

## Read First

- `ideas/open/772_lir_gep_pointer_authority_pr70460.md`
- `ideas/closed/764_lir_production_computed_goto_addr_value_publication.md`
- the `pr70460` production lowering route and existing `LirGepOp` verifier
  checks

## Landed Prerequisites

- 764 Step 1 (`9680b15b9`) is closed and accepted: its five computed-goto
  consumers pass 5/5 and are not this route's target.
- Existing GEP authority contracts must be inspected as constraints, not
  reopened or weakened without new first-bad-fact evidence.

## Non-Goals

- no Raw-BIR/importer changes or re-execution of 734 Step 7.24
- no 764 computed-goto carrier rework, accepted-prerequisite reimplementation,
  verifier relaxation, partial/raw authority, display-text recovery, failure
  exclusion, expectation downgrade, or baseline exception
- no broad rvalue, table, CFG, PHI, local/object, memory/va,
  aggregate/vector, target-lowering, MIR, or emission-family redesign

## Execution Rules

1. Reproduce `pr70460` before selecting a code seam; record the first owner
   that creates or forwards an empty `LirGepOp.ptr`.
2. Repair only that first evidenced seam from a typed current-function pointer
   result, preserving all fail-closed checks.
3. Add nearby production-path and malformed-authority coverage; do not turn
   the failure into an allowed or text-derived case.
4. Focused proof is not full-baseline acceptance. Return the exact handoff for
   a supervisor-owned fresh candidate after the bounded route is accepted.

## Ordered Steps

### Step 1 - Trace and repair the production GEP pointer authority loss

Goal: identify and minimally repair the first production seam that leaves
`LirGepOp.ptr` empty for `pr70460`.

Primary targets:

- the `pr70460` GEP producer/publication seam
- the first evidenced upstream owner if the GEP correctly copies an empty
  operand ID
- nearby `LirGepOp` verifier and focused production-path coverage

Actions:

- fresh-build and reproduce `pr70460`; trace the first empty-`ptr` owner
- publish only a verified typed pointer result into `LirGepOp.ptr`
- add or extend focused production-path and malformed-authority coverage
  without testcase-specific branching
- run fresh build and focused proof; report the producer seam, typed field,
  proof, and next baseline action to the supervisor

Completion check:

- `pr70460` no longer fails at empty `LirGepOp.ptr`; fresh build and focused
  positive/malformed proof pass with verifier contracts intact; the supervisor
  has an exact fresh-full-suite candidate handoff.
