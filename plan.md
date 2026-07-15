# LIR-To-New-BIR Inline-Assembly Output Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 816 scalar-integer output-only inline-assembly authority
handoff (`1b04a886e`).

## Purpose

Receive exactly the checked output-only scalar-integer inline-assembly result
published by closed 816. This is one bounded Raw-BIR receiver packet and does
not complete the remaining source-wide coverage matrix.

## Historical Progress

Steps 1 through 7.32 are accepted historical 734 work, most recently the
AMD64 aggregate VA-arg overflow receipt (`cf8c05985`). Do not repeat those
receiver rows or producer-authority work.

## Core Rule

Use only closed 816's native result ID, typed `Output` binding at index 0, and
`LirTypeRef`, subject to its current-function ownership checks. Never recover
semantic value or type from compatibility `result`, rendered operands,
templates, constraints, or testcase shape.

## Read First

- `ideas/closed/816_lir_next_residual_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (closed-816
  resumption record)
- `docs/lir_to_new_bir_remaining_coverage/successor_queue.md`

## Non-Goals

- Input, read/write, non-integer, multi-result, or any other inline-assembly
  form;
- producer/schema/verifier republishing, text/template/constraint parsing,
  target lowering, MIR/emission, or importer/dispatcher sweeps;
- remaining memory/VA, aggregate/vector, parameter, module/type/global,
  instruction, terminator, and documentation-convergence work.

## Ordered Steps

### Step 7.33 - Receive selected inline-assembly output-only authority

Goal: transactionally import closed 816's one scalar-integer output-only
`LirInlineAsmOp.ordinary_results[0]` row into a typed Raw-BIR destination.

Actions:

- consume only the fresh native result `LirValueId`, typed `Output` binding at
  index 0, and its `LirTypeRef` from `StmtEmitter::emit_inline_asm`;
- add only the minimum typed Raw-BIR destination, importer dispatch, reachable
  verifier work, and nearby positive/malformed-authority receiver coverage;
- reject missing, invalid, duplicate, role-mismatched, index-mismatched,
  type-mismatched, foreign, and all nonselected inline-assembly forms
  transactionally;
- run a fresh build and focused receiver proof before the supervisor-selected
  broader proof.

Completion check: exactly the selected output-only scalar-integer row imports
and verifies from native authority without presentation recovery. Every other
inline-assembly form remains fail closed, and source completion is reassessed
after this bounded receipt.
