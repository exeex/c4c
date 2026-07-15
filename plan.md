# LIR GEP Producer Result Authority Baseline Blocker Runbook

Status: Active
Source Idea: ideas/open/810_lir_gep_producer_result_authority_baseline_blocker.md
Resumed from: 796 accepted cast-result authority handoff (`387af7745`)

## Purpose

Complete the previously interrupted baseline gate for the bounded GEP
producer-authority blocker. Steps 1--2 remain accepted; Step 3 must establish
a fresh comparable full baseline before 810 can return control to 801.

## Core Rule

Keep the existing authoritative GEP contract intact. Do not derive authority
from rendered text, testcase identity, or instruction order, and do not
absorb anonymous layout, structured-call, or PHI work.

## Read First

- `ideas/open/810_lir_gep_producer_result_authority_baseline_blocker.md`
- its comparable-baseline residual ownership resumption record
- `ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`
  accepted return record
- canonical `test_before.log` and the fresh full-baseline procedure

## Non-Goals

- Repeating accepted 810 Steps 1--2 or reopening 796/795.
- Claiming 3037/3037 clearance from focused proof.
- Anonymous aggregate/direct-complex compatibility changes, PHI work, verifier
  weakening, text-derived identity, or generic provenance/pointer redesign.

## Ordered Steps

### Step 1 - Trace and classify failing GEP producer families (accepted)

Accepted in `f1cb9c510`: the bounded native pointer postfix and compound
add/sub producer family was traced. Do not repeat this step.

### Step 2 - Repair the selected GEP result-authority handoff (accepted)

Accepted in `1f1a1fb38`: only the selected native pointer producer family was
repaired. The later native parameter-index handoff is complete in `281737387`
and the cast-result handoff is accepted in `387af7745`; neither reopens this
step.

### Step 3 - Prove the blocker and return control to 801

Goal: obtain the fresh, comparable full-baseline evidence required after the
accepted 810, 795, and 796 handoffs.

Actions:

- run exactly `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure`;
- retain established focused proof only as supporting evidence, not baseline
  clearance;
- classify any residual failure by owned route without absorbing the unrelated
  anonymous aggregate/direct-complex dirty hunks or the 806 PHI residual;
- return control to 801 unchanged at Step 2 only after supervisor acceptance
  of a 3037/3037 comparable baseline.

Completion check: a fresh comparable full baseline passes 3037/3037 and the
supervisor accepts 810's bounded evidence. Any lesser or partial result does
not clear 810 or 801.
