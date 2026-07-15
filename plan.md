# Typed-LIR Parallel CFG-Edge Verifier Admission Runbook

Status: Active
Source Idea: ideas/open/787_lir_parallel_cfg_edge_verifier_admission.md
Switched from: ideas/open/734_lir_to_new_bir_container_completeness.md at unchanged Step 7.25

## Purpose

Unblock typed parallel CFG-edge input admission before the Raw-BIR PHI receiver
can prove exact edge-occurrence multiplicity.

## Goal

Make only the typed-LIR verifier/admission seam represent duplicate conditional
and switch successor occurrences faithfully, then return that contract to 734.

## Core Rule

Repeated typed successor IDs are ordered edge occurrences, not duplicate
authority to erase. Preserve all existing validity and ownership rejection;
never recover or classify semantics from presentation text.

## Read First

- `ideas/open/787_lir_parallel_cfg_edge_verifier_admission.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` resumption record
- `src/codegen/lir/verify.cpp` and the typed `LirCondBr`/`LirSwitch` definitions
- nearby LIR verifier tests and prior CFG authority history

## Non-Goals

- Raw-BIR receiver/container/importer work or acceptance of its current WIP
- broader producer/schema redesign, target lowering, or presentation recovery

## Ordered Steps

### Step 1 - Establish and admit typed parallel successor occurrences

Goal: revise the narrow typed-LIR verification boundary so duplicate
`LirCondBr` successor IDs and duplicate ordered `LirSwitch.case_successors`
are admitted as distinct parallel edge occurrences.

Primary targets:

- `src/codegen/lir/verify.cpp`
- the closest typed-LIR verifier tests for conditional and switch successors

Actions:

- identify the duplicate-specific rejection and retain independent validation
  of successor presence, validity, current-function ownership, and structural
  coherence
- permit repetition only as distinct ordered occurrences for the two named
  typed fields; do not change Raw-BIR or broader producer behavior
- add focused positive duplicate-successor coverage and malformed-neighbour
  rejection coverage without testcase-shaped matching or text recovery
- record the typed handoff required for parent 734's unchanged Step 7.25

Completion check:

- a fresh build and focused verifier proof establish ordered, multiplicity-
  preserving typed parallel occurrences with malformed authority still rejected.

### Step 2 - Return the narrow handoff

Goal: provide supervisor evidence that the blocker is complete and parent 734
can resume unchanged.

Actions:

- state the exact accepted fields and occurrence guarantees
- provide the focused proof and implementation commit
- state explicitly that no Raw-BIR receiver work was accepted by this blocker

Completion check:

- parent return is unambiguous: reactivate 734 at Step 7.25 and reattempt
  complete PHI receiver coverage including parallel occurrences.
