# Direct Call No-Prototype Variadic Signature Mismatch Runbook

Status: Active
Source Idea: ideas/open/189_direct_call_no_prototype_variadic_signature_mismatch.md

## Purpose

Clear the direct-call signature blocker discovered by the idea 188 milestone
validation gate.

## Goal

Make structured direct-call callee signature metadata compatible with valid C
old-style/no-prototype and variadic direct calls while preserving the stricter
prototype path.

## Core Rule

Repair call signature semantics; do not weaken tests, bypass verification
broadly, or replace structured call metadata with rendered text.

## Read First

- `ideas/open/189_direct_call_no_prototype_variadic_signature_mismatch.md`
- `todo.md` for the current packet pointer and delegated proof command.
- Direct-call signature construction and verification surfaces in the frontend,
  LIR, and LIR-to-BIR path.
- `test_after.log` only as historical context for the 9 failing test names; do
  not edit logs.

## Current Targets

- Structured callee signature metadata on direct calls.
- C old-style/no-prototype declaration and call handling.
- C variadic direct-call handling.
- Prototype direct-call mismatch checks that must remain strict.

## Non-Goals

- Do not begin backend restart implementation.
- Do not rewrite target MIR, assemblers, linkers, or object emission.
- Do not downgrade or weaken tests to make the 9 failures pass.
- Do not remove structured direct-call signature metadata for prototype calls.
- Do not broadly suppress `LirCallOp.callee_signature` verification.
- Do not perform unrelated call-lowering or backend refactors.

## Working Model

- Idea 188 is parked because milestone validation found 9 regressions against a
  green accepted baseline.
- The common observed error is `LirCallOp.callee_signature: structured callee
  signature does not match call arguments`.
- Valid C no-prototype and variadic direct calls need intentional structured
  representation instead of a fixed-arity prototype signature check.
- Ordinary prototype direct calls should keep strict structured metadata and
  mismatch detection.

## Execution Rules

- Keep routine evidence and intermediate findings in `todo.md`.
- The executor must run the exact proof command delegated by the supervisor.
- Treat named-test shortcuts and expectation rewrites as route failures.
- Preserve structured direct-call metadata and only adjust the semantic model
  for no-prototype and variadic flexibility.
- If investigation reveals a separate blocker outside direct-call
  no-prototype/variadic signature semantics, record it in `todo.md` and return
  to the supervisor for lifecycle routing.

## Step 1: Locate Signature Construction And Failing Shape

Goal: identify where direct-call callee signatures are created, stored, and
verified for prototype, no-prototype, old-style, and variadic calls.

Concrete actions:
- Inspect the direct-call lowering path and `LirCallOp.callee_signature`
  verifier.
- Reproduce or inspect at least one no-prototype/old-style failure and one
  variadic failure under the supervisor-delegated narrow command.
- Determine whether the mismatch comes from signature construction, call
  argument modeling, verifier rules, or a conversion boundary.
- Record the concrete failing shape and owned implementation surface in
  `todo.md`.

Completion check:
- `todo.md` names the failing call categories, the structured signature values
  involved, and the implementation surface selected for repair.

## Step 2: Repair No-Prototype And Variadic Semantics

Goal: implement the smallest semantic repair that represents valid flexible C
direct calls without weakening prototype-call checking.

Concrete actions:
- Add or adjust structured metadata so no-prototype/old-style calls are not
  forced through an exact fixed-arity prototype signature contract.
- Add or adjust structured metadata so variadic calls distinguish fixed
  parameters from accepted variadic tail arguments.
- Keep prototype calls on the strict structured signature path.
- Avoid rendered-name or printed-signature authority for semantic decisions.

Completion check:
- The known no-prototype/old-style and variadic failure samples no longer hit
  the structured callee signature mismatch, while prototype-call behavior is
  unchanged.

## Step 3: Prove The Blocker Fix

Goal: prove the direct-call blocker is resolved under the delegated narrow and
broader validation commands.

Concrete actions:
- Run the exact supervisor-delegated proof command covering the 9 failing tests
  and nearby direct-call signature cases.
- Preserve canonical validation state according to supervisor policy.
- Record proof commands and results in `todo.md`.
- Do not claim closure from one named testcase alone.

Completion check:
- The delegated blocker proof is green and `todo.md` records the exact command,
  result, and remaining risk.

## Step 4: Return To Freeze Gate

Goal: hand lifecycle control back to the idea 188 freeze closure gate after the
blocker is fixed.

Concrete actions:
- Confirm the blocker acceptance criteria are satisfied.
- Record in `todo.md` that idea 188 should be reactivated for milestone
  validation.
- Do not close idea 188 from this runbook.
- Let the supervisor decide whether broader/full validation is already
  sufficient or whether idea 188 Step 3 must rerun.

Completion check:
- `todo.md` contains a clear `return to idea 188` note with blocker proof
  evidence and no backend restart work has begun.
