# LIR DirectScalar Body-Parameter Producer/Verifier Publication Runbook

Status: Active
Source Idea: ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md
Resumed from: Ideas 821 and 822 focused frontend proof boundary

## Purpose

Complete the remaining DirectScalar boundary proof for the accepted typed
`ull` authority publication, then record only the selected 734 handoff.

## Historical Progress

Step 1 trace was accepted in `35cff0993`. Step 2 typed DirectScalar
publication and focused fail-closed coverage were accepted in `4bcc7c8ff`.
Resume at Step 3; do not repeat or broaden those steps.

## Core Rule

Publish and verify only structured native current-function DirectScalar
identity and type. Do not derive authority from presentation fields, change
Raw-BIR, or admit generic scalar parameter receipt.

## Read First

- `ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md`
- `ideas/open/821_frontend_lir_manual_switch_modelled_result_authority.md`
- `ideas/open/822_lowering_produced_lir_switch_selector_authority.md`
- `src/codegen/lir/verify.cpp` and the existing `ull` native DirectScalar
  producer path

## Non-Goals

- Accepting or modifying either pending Idea 821 manual-fixture patch or Idea
  822 lowering-selector patch.
- Raw-BIR/importer work, generic scalar parameters, other ABI forms, generic
  switch redesign, or broader DirectScalar authority redesign.

## Ordered Steps

### Step 3 - Prove the boundary and record the 734 handoff

Goal: demonstrate that the existing DirectScalar verifier failure no longer
blocks the external route, then return only the selected receiver authority to
Idea 734.

Actions:

- preserve the unaccepted Idea 821 and 822 shared-worktree patches while
  diagnosing and repairing only the remaining DirectScalar route;
- run a fresh build and focused producer/verifier proof;
- run `ctest --test-dir build --output-on-failure -R
  '^llvm_gcc_c_torture_src_20041011_1_c$'`;
- run the supervisor-selected matching regression/broader proof;
- record that 734 resumes solely at Step 7.35 with its selected
  `LirBinOp.lhs` row, only after the checkpoint succeeds.

Completion check: the external test no longer stops at the named pre-import
DirectScalar verifier failure under accepted prerequisites, the selected
broader proof is executable and accepted, and the handoff grants no generic
scalar authorization. A composite-only pass that depends on unaccepted Ideas
821/822 selector work or unaccepted 734 receiver work is diagnosis only; a
backend command with missing registered executables is not broader proof.
