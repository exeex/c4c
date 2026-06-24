# RV64 Inline Asm Custom Vector Integration Review

Status: Active
Source Idea: ideas/open/347_rv64_inline_asm_custom_vector_integration_review.md
Activated from: ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md

## Purpose

Review whether the completed inline-asm children compose into the custom vector
workflow promised by the umbrella.

Goal: decide whether the parent umbrella is ready for close review, or whether
a concrete follow-up child idea is required.

## Core Rule

This is a final integration review runbook, not permission for broad new
implementation. If review exposes a real missing capability, create or request
a focused child idea instead of silently expanding this plan.

## Read First

- `ideas/open/347_rv64_inline_asm_custom_vector_integration_review.md`
- `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`
- `ideas/closed/346_rv64_standard_insn_scalar_inline_asm_object_route.md`
- `ideas/closed/341_rv64_vector_register_inline_asm_constraints_stage2.md`
- `ideas/closed/342_rv64_ev_insn_d_inline_asm_stage3.md`
- `ideas/closed/343_rv64_consteval_inline_asm_template_strings.md`

## Current Scope

- Map umbrella completion criteria to closed child evidence and current tests.
- Verify the composed path from compile-time asm template to RV64 object bytes.
- Confirm vector group allocation and base-register substitution are exercised
  in the `.insn.d` route.
- Record any uncovered gap as a follow-up child idea instead of folding it into
  the review.

## Non-Goals

- Do not add EV operation mnemonics or intrinsics.
- Do not implement broad RVV semantic lowering.
- Do not add named GNU operands, `%c[...]` modifiers, mask constraints, or full
  GNU assembler compatibility unless a separate child idea is created.
- Do not weaken existing inline asm, vector constraint, `.insn.d`, consteval,
  or object tests.
- Do not close the parent umbrella from this runbook without plan-owner close
  review.

## Working Model

Treat each completed child as a proof input. The review should identify the
smallest current fixture or test set that demonstrates composition across
frontend constant folding, inline asm metadata, vector constraint binding,
prepared substitution, `.insn.d` encoding, and object emission.

## Execution Rules

- Prefer inspection plus focused proof over new implementation.
- If a test already proves the composed behavior, record the exact test and
  assertion surface in `todo.md`.
- If proof is missing but the capability exists, add only the narrowest review
  fixture needed after supervisor delegation.
- If capability is missing, stop and request a focused follow-up source idea.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.

## Step 1: Map Closed Child Evidence

Goal: determine whether every umbrella completion criterion has completed child
coverage.

Actions:

- Read the parent umbrella and closed child closure notes.
- Map each umbrella stage and completion criterion to the child that claims it.
- Identify the current tests that prove each completed surface.
- Record any missing criterion or stale dependency in `todo.md`.

Completion check: `todo.md` contains a criterion-to-evidence map and either
confirms Step 2 can proceed or names the specific gap that needs a follow-up
child.

## Step 2: Verify Composed Route Fixtures

Goal: prove that the accepted child surfaces compose in the current tree.

Actions:

- Locate representative tests or fixtures for literal `.insn.d`,
  compile-time-generated `.insn.d`, `VRM*` operands, and object-byte output.
- Confirm the compile-time template route reaches the same inline asm lowering,
  operand metadata, substitution, and RV64 object emission path as literals.
- Confirm grouped vector operands encode their base register and preserve
  overlap/tied semantics in the proved route.
- If a narrow integration fixture is missing but the capability exists, prepare
  a small follow-up packet for supervisor/executor approval.

Completion check: focused proof identifies the composed route and either passes
without code changes or records the exact missing review fixture.

## Step 3: Run Closure-Quality Validation

Goal: provide enough proof for plan-owner closure decisions.

Actions:

- Run the supervisor-selected broader validation subset for inline asm,
  frontend constant templates, backend vector constraints, and RV64 object
  emission.
- Compare proof against the current baseline with the regression guard when
  directed by the supervisor.
- Record the command, result, and log names in `todo.md`.

Completion check: validation passes, or failures are classified as blockers or
separate follow-up ideas.

## Step 4: Decide Parent Umbrella Readiness

Goal: decide the next lifecycle action after this child.

Actions:

- If all criteria are covered and proof is green, ask plan owner to close this
  child and then review the parent umbrella for closure.
- If a required capability is missing, create or request one focused child idea
  and do not close the parent umbrella.
- If only optional ergonomic work remains, record it as out-of-scope follow-up
  and keep parent closure separate.

Completion check: `todo.md` states whether the parent umbrella is ready for
close review or names the next concrete child idea.
