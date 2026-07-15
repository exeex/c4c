# DirectScalar Binary-LHS Authority Return-Decision Runbook

Status: Active
Source Idea: ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md
Resumed from: ideas/closed/828_lir_direct_scalar_unary_fneg_authority.md, Step 3

## Purpose

Make the parent 827 repair/close decision after the accepted unary `fneg`
blocker return, without reviving its disproved binary mismatch premise.

## Core Rule

Do not treat the resolved unary `fneg` failure as binary-LHS evidence. Do not
infer an in-scope binary defect from rendered types, names, diagnostics, or
testcase shape.

## Read First

- `ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md`
- `ideas/closed/828_lir_direct_scalar_unary_fneg_authority.md`
- `ideas/open/825_lir_next_body_parameter_authority_handoff.md`

## Non-Goals

- New binary producer edits without a newly observed in-scope first bad fact.
- Direct switch-selector implementation, Raw-BIR/importer, generic rows, and
  the preserved dirty Idea 825 worktree slice.

## Ordered Steps

### Step 2 - Decide the disproved binary-LHS route

Goal: decide whether the source is intentionally concluded or has a newly
observed binary-LHS repair route after 828's accepted return.

Actions:

- preserve the accepted fact that the original first failing operation was
  unary `fneg`, not a binary definition/operation type mismatch;
- do not credit the dirty aggregate `frontend_hir_tests` SEGFAULT to 828: clean
  detached before/after `524b24f64` both pass it, while existing Idea 825 owns
  the dirty selector-authority route;
- conclude 827 only if no new binary-LHS first bad fact exists; otherwise
  repair this runbook from that fact without widening into 825.

Completion check: an explicit close or in-scope repaired runbook is recorded.
If concluded, resume Idea 825 at its recorded Step 2 to own its dirty route.
