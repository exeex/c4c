# RV64 Inline Asm Custom Vector Encoding Umbrella Runbook

Status: Active
Source Idea: ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md

## Purpose

Finish the umbrella workflow for RV64 custom vector inline asm by deriving the
remaining child work, keeping completed stages archived, and closing the
umbrella only after the composed C++ custom-vector path is reviewed.

## Goal

Create and drive the remaining consteval/template asm string child work, then
perform a final integration review proving the completed stages compose into
the intended user-facing custom vector instruction workflow.

## Core Rule

Do not close the umbrella until all child ideas created from it are closed and
final review confirms that C++ user/library code can define custom vector
instruction families without adding a compiler-known mnemonic or intrinsic for
each EV operation.

## Read First

- `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`
- `ideas/closed/340_rv64_standard_insn_inline_asm_stage1.md`
- `ideas/closed/341_rv64_vector_register_inline_asm_constraints_stage2.md`
- `ideas/closed/342_rv64_ev_insn_d_inline_asm_stage3.md`

## Current Scope

- Treat Stage 1, Stage 2, and Stage 3 as completed foundations.
- Create the remaining child idea for C++ constant-evaluated inline asm
  template strings.
- Preserve the umbrella as the durable integration contract.
- After the final child closes, run an integration review against the umbrella
  completion criteria.

## Non-Goals

- Do not reopen scalar `.insn`, vector register constraints, or positional
  `.insn.d` unless the final string route exposes a real integration defect.
- Do not add named GNU operands, `%c[...]` modifiers, vector mask conventions,
  broad RVV lowering, or EV compiler intrinsics as part of the final string
  route.
- Do not accept runtime-generated asm template strings.
- Do not weaken existing inline asm, vector constraint, or object emission
  tests to make umbrella closure appear complete.

## Working Model

- The umbrella remains active only to coordinate remaining child creation and
  final integration review.
- Implementation work should live in a focused child idea whenever it is more
  specific than umbrella coordination.
- Completed child artifacts are evidence for final review, not active work to
  churn.

## Execution Rules

- Use `todo.md` for packet state and proof notes.
- Use plan-owner lifecycle work to create or switch to a new child idea.
- Use executor packets for implementation after a child idea has an active
  runbook.
- Keep proof target-owned: inline asm string folding must feed the same
  compiler path as literal asm templates, and object proof must remain c4c
  owned.
- Treat testcase-shaped shortcuts, expectation downgrades, or raw string
  acceptance without compile-time folding as blocking route failures.

## Step 1: Create Final Consteval String Child Idea

Goal: Create a focused source idea for C++ constant-evaluated inline asm
template strings.

Primary target:

- `ideas/open/<next>_rv64_consteval_inline_asm_template_strings.md`

Actions:

- Derive the child from the umbrella final-stage requirements.
- Include accepted compile-time string forms, rejection of runtime strings,
  `.insn.d` integration expectations, proof ladder, and reviewer reject
  signals.
- Keep Stage 1, Stage 2, and Stage 3 as dependencies rather than re-scoping
  their completed work.
- After creation, switch lifecycle state to the new child if it is immediately
  activatable.

Completion check:

- A new child idea exists under `ideas/open/` with concrete scope, acceptance
  criteria, proof expectations, and reviewer reject signals.
- `plan.md` and `todo.md` are switched or reset according to the lifecycle
  rules if the child becomes the active plan.

## Step 2: Implement Final Consteval String Route

Goal: Execute the child idea that accepts compile-time-produced asm template
strings.

Primary targets:

- Frontend constant-expression or fixed-string folding surfaces selected by
  the child idea.
- Inline asm parser/HIR/LIR carrier paths that currently expect a literal
  template string.
- RV64 `.insn.d` tests that compare consteval/template-produced templates
  against the literal route.

Actions:

- Fold accepted compile-time string expressions into the same final inline asm
  template text as raw string literals.
- Preserve existing constraints, operand ordering, volatile/clobber metadata,
  and `.insn.d` object encoding behavior.
- Reject runtime-generated asm strings with a clear diagnostic.
- Add focused positive and negative tests according to the child runbook.

Completion check:

- The child idea closes through its own lifecycle gate.
- Tests prove template-built EV instruction helpers produce the same emitted
  bytes as literal `.insn.d` templates.
- Runtime strings remain rejected.

## Step 3: Final Umbrella Integration Review

Goal: Confirm the closed child stages compose into the umbrella's intended
C++ custom-vector workflow.

Actions:

- Review Stage 1 scalar `.insn` support, Stage 2 vector constraints, Stage 3
  `.insn.d` object emission, and the final consteval string child together.
- Check the umbrella completion criteria directly.
- Verify no completed stage depends on external assembler proof as the primary
  object route and no EV operation was added as a compiler-known mnemonic to
  claim custom instruction support.
- Run or request the smallest credible composed validation set, escalating
  beyond backend-only if the final string route touched frontend/HIR/LIR paths.

Completion check:

- A reviewer or supervisor-side review records that all umbrella completion
  criteria are met.
- Regression guard passes for the chosen closure scope.
- The umbrella source idea is ready to move from `ideas/open/` to
  `ideas/closed/`.
