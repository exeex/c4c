# BIR Direct-Call Semantic Boundary Evidence Runbook

Status: Active
Source Idea: ideas/open/562_bir_direct_call_semantic_boundary_evidence.md

## Purpose

Name the missing semantic direct-call fact behind retained
`semantic_lir_to_bir_direct_call_family` rows before any call lowering repair
proceeds.

## Goal

Reproduce one retained direct-call representative, identify the exact absent or
incoherent BIR-owned fact, then repair only that named boundary if the evidence
justifies implementation.

## Core Rule

Do not recover direct-call argument, return, byval, or sret facts from RV64
target-specific call shapes. RV64 call lowering must consume BIR or prepared
facts, and BIR ownership must be proven from focused call-publication evidence.

## Read First

- `ideas/open/562_bir_direct_call_semantic_boundary_evidence.md`
- Existing BIR route6/printer call-publication tests and dumps
- `BirFunctionLowerer::lower_call_inst` and nearby direct-call callee,
  signature, return ABI, argument lowering, byval/sret, and
  `CallArgumentSourceRelationship` publication code
- Current RV64 gcc torture artifacts for representatives such as
  `src/20000419-1.c`, `src/20000717-1.c`, `src/20040703-1.c`, and
  `src/pr67226.c`, if present

## Current Targets

- Retained rows diagnosed as `semantic_lir_to_bir_direct_call_family`.
- Direct-call facts owned by BIR semantic call lowering:
  callee classification, signature and return ABI, argument source
  publication, call-result publication, byval/sret aggregate layout facts, and
  prepared handoff readiness.
- Same-snapshot semantic BIR and, if reached, prepared BIR evidence for one
  representative direct-call row.

## Non-Goals

- Do not change RV64 call lowering before the missing BIR or prepared fact is
  named.
- Do not combine this route with local-memory base-plus-offset work.
- Do not treat no-count call-metadata suspicions as implementation-ready.
- Do not weaken unsupported accounting, expected output, diagnostics, or
  semantic admission checks.
- Do not add named-case shortcuts for any retained torture representative.
- Do not broadly rewrite `lower_call_inst` outside the named first bad
  direct-call fact.

## Working Model

- If semantic BIR lacks or mispublishes callee, argument, return, byval, sret,
  or call-result facts, the first owner is BIR direct-call publication.
- If semantic BIR publishes the needed fact but prepared handoff rejects or
  loses it, the first owner is the prepared handoff boundary.
- If the evidence cannot distinguish those fact families, the rows remain an
  evidence gap rather than becoming an implementation route.

## Execution Rules

- Keep routine reproduction notes, commands, failing facts, dump paths, and
  proof results in `todo.md`.
- Add focused BIR route6/printer coverage before relying on representative
  RV64 proof for any code-changing repair.
- Prefer `src/20000717-1.c` or `src/pr67226.c` as the first representative,
  with `src/20000419-1.c` or `src/20040703-1.c` as fallbacks.
- Treat diagnostic renames, expectation rewrites, unsupported downgrades, and
  helper-only refactors as non-progress.
- Escalate to plan-owner split if evidence proves separate callee,
  argument-source, return-result, byval/sret, or prepared-handoff initiatives.

## Steps

### Step 1: Reproduce Direct-Call Boundary Evidence

Goal: capture same-snapshot semantic BIR and, if reached, prepared BIR/RV64
evidence for one retained representative.

Primary target: `src/20000717-1.c` or `src/pr67226.c`.

Actions:
- Find or rerun the current representative command through semantic BIR.
- Continue through prepared BIR and RV64 object routing only as far as the
  route naturally reaches.
- Record the exact command, representative, diagnostic text, failing call, and
  relevant dump paths in `todo.md`.
- Identify the `CallInst` and direct-call family row that corresponds to the
  failing semantic boundary.
- Stop without implementation if the reproduction cannot tie one failing call
  to a visible semantic BIR fact family.

Completion check:
- `todo.md` names the representative, command, failing call, diagnostic,
  semantic BIR evidence location, and any prepared/RV64 rejection point reached.

### Step 2: Classify The Missing Direct-Call Fact

Goal: decide whether the first bad fact is callee classification, argument
source publication, return result publication, byval/sret handling, prepared
handoff, or still an evidence gap.

Primary target: `BirFunctionLowerer::lower_call_inst` evidence and focused
route6 call records.

Actions:
- Inspect `CallInst` argument and return ABI rows, `call_arg_source`
  annotations, route6 call argument source/producer/publication records,
  call-result records, and byval/sret aggregate layout facts.
- Classify BIR-owned only if the required direct-call fact is absent,
  incoherent, or mispublished before prepared handoff.
- Classify prepared-handoff-owned only if the BIR fact is present and later
  rejected or lost by the prepared boundary.
- Record the classification and exact first bad fact in `todo.md`; if no owner
  is proven, preserve the evidence gap and request route review.

Completion check:
- `todo.md` contains one first-owner classification tied to visible same-run
  evidence, or explicitly records why the route is blocked as an evidence gap.

### Step 3: Add Focused Call-Publication Coverage

Goal: make the proven direct-call fact observable in focused BIR coverage
before changing the repair surface.

Primary target: the smallest BIR route6/printer test that exposes the missing
callee, argument, return, byval/sret, or prepared-handoff fact.

Actions:
- Add or extend focused BIR call-publication coverage for the classified fact.
- Keep the test independent of the named torture case when possible.
- Prove the focused test fails before the repair or document why existing
  coverage already exposes the exact fact.
- Do not start implementation until the proof surface is clear.

Completion check:
- Focused coverage exists for the classified direct-call boundary and
  `todo.md` records the exact proof command and result.

### Step 4: Repair Only The Named Direct-Call Boundary

Goal: fix the proven direct-call semantic producer or prepared-handoff gap
without target-shaped reconstruction.

Primary target: the code surface classified in Step 2.

Actions:
- For BIR-owned failures, repair publication of the named callee, argument,
  return, byval/sret, or call-result fact.
- For prepared-handoff failures, repair the handoff so it preserves and
  consumes the already-published BIR fact.
- Keep diagnostics fail-closed for malformed, unsupported, or incomplete calls.
- Avoid broad call-route rewrites and named-case checks.

Completion check:
- The focused coverage from Step 3 passes, and the diff shows a semantic
  direct-call boundary repair rather than an expectation or diagnostic-only
  change.

### Step 5: Prove Representative And Adjacent Call Behavior

Goal: show that the repair advances representative RV64 behavior and does not
regress adjacent BIR/prepared call paths.

Primary target: the representative from Step 1 plus supervisor-selected focused
BIR/prepared call tests.

Actions:
- Rerun the representative command from Step 1.
- Run the focused test subset from Step 3 and the supervisor-selected broader
  call subset.
- Record commands and results in `todo.md`.
- If the representative still fails for a different first bad fact, record the
  new owner instead of expanding the slice silently.

Completion check:
- `todo.md` contains fresh proof for focused coverage and representative
  behavior, plus any remaining first-owner facts that require a separate route.
