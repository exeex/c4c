# LIR PHI Special-Token Semantic Authority Publication Runbook

Status: Active
Source Idea: ideas/open/786_lir_phi_special_token_semantic_authority_publication.md
Activated from: paused 734 Step 7.25 special-token authority blocker

## Purpose

Repair the upstream LIR authority missing from PHI SpecialToken operands before
the Raw-BIR receiver resumes.

## Goal

Give every relevant PHI SpecialToken operand native semantic authority that is
independent of its display spelling.

## Core Rule

`LirOperandKind::SpecialToken` classification and `LirOperand::str()` are not
semantic inputs. Publish and verify a typed authority at the LIR producer
boundary; do not recover it downstream.

## Read First

- `ideas/open/786_lir_phi_special_token_semantic_authority_publication.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` resumption record
- `src/codegen/lir/operands.hpp`
- relevant PHI construction and `src/codegen/lir/verify.cpp` seams

## Non-Goals

- no Raw-BIR/importer/receiver work, backend lowering, MIR, emission, or
  target interpretation
- no text-derived semantic recovery or broad operand redesign

## Execution Rules

1. Keep the carrier limited to the existing SpecialToken forms required by
   relevant PHI rows.
2. Treat display spelling as a compatibility mirror only.
3. Reject absent, mismatched, invalid, and misleading-display authority in the
   LIR verifier before any consumer relies on it.
4. Prove neighboring producer and malformed-authority cases; do not treat a
   Raw-BIR test as proof for this producer/verifier step.
5. At completion, document the exact handoff to 734; do not modify 734's
   receiver during this plan.

## Ordered Steps

### Step 1 - Publish and verify PHI SpecialToken authority

Goal: introduce the smallest typed authority for existing SpecialToken PHI
operands and establish producer/verifier enforcement.

Primary targets:

- `src/codegen/lir/operands.hpp`
- relevant PHI producers, `src/codegen/lir/verify.cpp`, and focused tests

Actions:

- enumerate the existing classified token forms used by relevant PHI inputs
  and encode their identity as a native authority alternative
- publish that authority from the relevant PHI producers and enforce coherent
  kind/authority in verification without consulting display text
- add nearby positive and malformed-authority coverage, including a
  misleading-display rejection, and record a precise 734 handoff

Completion check:

- fresh build plus focused producer/verifier proof show the selected PHI
  SpecialToken authority is native and fail-closed; no Raw-BIR/importer diff is
  present.
