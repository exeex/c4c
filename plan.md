# LIR Next Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/818_lir_next_body_parameter_authority_handoff.md
Resumed from: accepted 819 scalar binary-LHS producer prerequisite (`b16935c69`)

## Purpose

Publish one exact, receiver-ready structured authority contract for a remaining
function-body parameter use, then return control to 734 without implementing
the Raw-BIR receipt.

## Core Rule

Select only native current-function facts that production and verification can
structurally establish. Presentation text, parameter names, signature
rendering, raw operands, diagnostics, and testcase shape are never authority.

## Read First

- `ideas/open/818_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (post-Step 7.34
  resumption record)
- `ideas/closed/817_lir_body_parameter_receiver_authority_handoff.md`
- `ideas/open/795_lir_body_parameter_authority_handoff.md` (separate accepted
  route; do not reuse it as 734 authority)

## Non-Goals

- Raw-BIR, importer, dispatcher, or receiver-test changes;
- reopening accepted direct-pointer or parameter-index contracts;
- broad ABI work or multiple parameter forms; and
- any non-parameter source family.

## Ordered Steps

### Step 1 - Trace and select one body-parameter use authority row

Goal: identify one native body-parameter use whose value identity, position,
type, owner, and ABI classification can be proven without presentation
recovery.

Actions:

- inspect only the relevant production/schema/verifier seams;
- document the exact selected row and rejected neighboring forms in the active
  execution state before making a producer change;
- re-evaluate only whether the accepted 819 scalar `LirBinOp.lhs` tuple is a
  receiver-ready row; if not, stop and return a separately scoped blocker
  rather than broadening this idea.

Completion check: one bounded candidate and its structural authority sources
are explicit; no Raw-BIR receiver work is selected.

### Step 2 - Publish and verify the selected authority contract

Goal: add only the selected native fields and verifier admission needed for a
later 734 receiver handoff.

Actions:

- publish current-function identity, parameter position, type, owner, and ABI
  classification only where the selected row proves them;
- reject missing, invalid, foreign, duplicate, type-incoherent, or
  ABI-incoherent forms before downstream use;
- add nearby positive and malformed-authority producer coverage.

Completion check: one selected row has a checked structured contract and all
nonselected forms remain fail-closed.

### Step 3 - Prove and hand off the bounded producer route

Goal: record the exact receiver fields, rejection boundary, focused proof, and
734 return point.

Actions:

- run a fresh build and focused same-feature producer proof;
- provide the supervisor-selected matching regression guard if required;
- update the source resumption record with the selected handoff and require
  reactivation of 734 at Step 7.35 only.

Completion check: accepted proof and an exact one-row handoff exist; no
Raw-BIR receipt or source-wide completion claim is made.
