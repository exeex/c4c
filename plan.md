# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: accepted Steps 1 through 7.24; accepted safe partial Step 7.25
receiver `006d79aaf`; closed 751 value/predecessor authority (`6ece9fe8f`);
closed 786 native PHI SpecialToken authority (`91b5bde43`); and closed 788
successor-occurrence authority (`f52ced6ae`, `10d63ff7a`).

## Purpose

Resume the Raw-BIR receiver route at unchanged Step 7.25 without resetting
accepted receiver work. Only the remaining exact parallel-edge PHI portion is
in scope.

## Goal

Complete the bounded parallel-edge portion of the structured `LirPhiOp` Raw-BIR
receiver using the native per-incoming successor occurrence, with no
presentation recovery or partial publication.

## Core Rule

Use only the typed `LirPhiOp` result/type, closed 751's incoming value and
current-function predecessor, closed 786's native SpecialToken authority, and
closed 788's `LirPhiIncoming.successor_occurrence`
`LirSuccessorOccurrenceId`. The ID is interpreted against the typed
predecessor terminator; labels, spellings, printer output, LLVM text, PHI input
order, and instruction order are never semantic inputs.

## Landed Progress And Boundary

- Steps 1 through 7.24 and the unambiguous/loop-backedge partial Step 7.25
  receiver in `006d79aaf` are accepted and must not be repeated.
- 788's selected ternary, logical, AArch64-vaarg, and AMD64-vaarg producers
  now publish the exact occurrence ID. Its verifier requires present, valid,
  current-function ownership, predecessor/destination coherence, and complete
  unique destination-edge occurrence coverage with multiplicity/order.
- This step owns only Raw-BIR receiver/container/importer/verifier work and
  nearby transactional coverage for the remaining parallel-edge cases.

## Non-Goals

- no LIR producer/schema/verifier changes, target lowering, MIR, emission,
  legacy-BIR, canonicalization, or presentation recovery
- no reimplementation of accepted unambiguous/loop-backedge PHI work
- no local/object, memory/va, aggregate/vector, body-parameter, or other later
  receiver family

## Execution Rules

1. Extend only the remaining parallel-edge receipt using the carried exact
   `successor_occurrence`; preserve one incoming per exact terminator occurrence.
2. Preserve native incoming order and repeated predecessor blocks. Conditional
   true/false and switch default/case occurrences stay distinct when their
   destinations are equal.
3. Fail closed before publication for absent, invalid, foreign, incoherent,
   duplicate, or incomplete occurrence authority, and retain transactional
   rollback.
4. Prove typed positive ternary/logical/AArch64-vaarg/AMD64-vaarg parallel-edge
   receipt and nearby malformed authority rejection. Do not use rendered text
   or testcase identity as a semantic probe.
5. Run a fresh build and focused proof. The supervisor owns regression logs and
   broader/full acceptance. Return this source to its completion gate after the
   bounded receiver; do not infer closure.

## Ordered Steps

### Step 7.25 - Receive typed PHI incoming authority

Goal: complete only the remaining parallel-edge exact-occurrence receipt into
the existing typed Raw-BIR PHI route.

Actions:

- consume `LirPhiIncoming.successor_occurrence` alongside the already accepted
  typed result/type, incoming value/predecessor, and SpecialToken authorities;
- map each remaining incoming through current-function Raw-BIR maps and its
  exact predecessor terminator successor occurrence without collapsing repeats;
- validate ownership, types, predecessor/destination coherence, exact
  occurrence selection, uniqueness, multiplicity/order, and rollback before
  publication;
- add focused positive parallel conditional/switch coverage across the selected
  producer populations and malformed missing/invalid/foreign/mismatched/
  duplicate/incomplete authority coverage.

Completion check: a fresh build and focused positive/negative proof establish
the remaining typed parallel-edge PHI receipt without presentation recovery,
while preserving accepted `006d79aaf` work.
