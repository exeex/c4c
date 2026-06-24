# RV64 Consteval Inline Asm Template Strings Runbook

Status: Active
Source Idea: ideas/open/343_rv64_consteval_inline_asm_template_strings.md
Activated from: ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md

## Purpose

Enable C++ compile-time-produced inline asm template strings to feed the same
compiler path as literal inline asm templates, completing the user/library side
of RV64 EV custom vector instruction support.

## Goal

Accept a narrow, explicit compile-time string surface for inline asm templates,
reject runtime strings, and prove template-built `.insn.d` helpers emit the
same bytes as literal `.insn.d` templates.

## Core Rule

Fold accepted asm-template expressions to final string text before inline asm
lowering. After folding, the existing parser, metadata, constraint,
substitution, and object emission paths must see the same template text they
would see for a literal.

## Read First

- `ideas/open/343_rv64_consteval_inline_asm_template_strings.md`
- `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`
- `ideas/closed/340_rv64_standard_insn_inline_asm_stage1.md`
- `ideas/closed/341_rv64_vector_register_inline_asm_constraints_stage2.md`
- `ideas/closed/342_rv64_ev_insn_d_inline_asm_stage3.md`

## Current Targets

- Frontend asm-statement parsing or semantic analysis surface that currently
  requires a literal template string.
- Existing constant-expression or compile-time string representation, if one
  exists.
- Inline asm HIR/LIR/BIR carrier paths that store final template text and
  operand metadata.
- RV64 `.insn.d` tests that can compare consteval/template-built templates
  against literal-template object bytes.

## Non-Goals

- Do not accept runtime-generated asm strings.
- Do not add Python-style formatting, `std::format`-style formatting, named GNU
  operands, or `%c[...]` modifiers.
- Do not reopen completed `.insn`, `VR`/`VRM2`/`VRM4`, or `.insn.d` behavior
  except for real integration defects exposed by constant-folded templates.
- Do not add EV intrinsics or compiler-known EV mnemonics to claim progress.
- Do not weaken existing inline asm or object tests.

## Working Model

- Keep the first accepted compile-time string surface narrow and aligned with
  existing frontend capabilities.
- Prefer compile-time `+` concatenation of literals and supported fixed-string
  values.
- Accept `constexpr` or `consteval` helper results only when they fold to final
  template text before inline asm lowering.
- Route unsupported compile-time string shapes to diagnostics rather than
  silently accepting an empty, runtime, or unevaluated template.

## Execution Rules

- Inspect existing parser, sema, constant-expression, HIR/LIR, and inline asm
  tests before choosing the exact helper shape.
- Make every positive case prove equivalence with a literal template route.
- Keep negative diagnostics specific enough to distinguish non-constant asm
  templates from malformed `.insn.d` or constraint failures.
- Preserve operand ordering, constraint strings, clobbers, volatility, memory
  effects, and `%0` positional placeholder semantics.
- Use focused tests first, then run the supervisor-selected broader subset
  because this route may cross frontend and backend surfaces.

## Step 1: Map Existing String And Inline Asm Surfaces

Goal: Identify the narrowest existing representation that can carry a
compile-time string expression into inline asm.

Primary targets:

- Parser/sema handling for `asm volatile(...)`.
- Constant-expression or fixed-string support already present in the frontend.
- Tests for string literal folding, concatenation, constexpr arrays, or
  consteval helper results.

Actions:

- Inspect where inline asm currently requires a raw string literal.
- Inspect whether c4c already represents compile-time string values or string
  literal concatenation.
- Choose one supported first surface for compile-time concatenation and one
  helper-style surface only if it fits the existing machinery.
- Record any unsupported but tempting string forms in `todo.md`; do not widen
  the source idea.

Completion check:

- The executor can name the selected compile-time string representation and the
  exact frontend location where it must fold to final asm-template text.
- No implementation route depends on runtime string values.

## Step 2: Fold Accepted Template Expressions Before Inline Asm Lowering

Goal: Convert accepted asm-template expressions into final template text before
the existing inline asm path consumes them.

Primary targets:

- The frontend parser/sema boundary that builds the inline asm statement.
- The HIR/LIR carrier for inline asm template text.

Actions:

- Preserve raw literal behavior unchanged.
- Add folding for the selected compile-time string expression forms.
- Emit a clear diagnostic for runtime or unsupported template expressions.
- Keep folded output indistinguishable from the literal text for downstream
  parsing and metadata.

Completion check:

- Focused frontend/HIR tests show literal and folded template forms produce the
  same final template string, operands, constraints, clobbers, volatility, and
  memory metadata.
- Negative tests prove runtime strings and unsupported expression forms are
  rejected.

## Step 3: Prove RV64 `.insn.d` Helper Integration

Goal: Prove a user/library-style compile-time helper can build an EV `.insn.d`
template that emits the same object bytes as a literal template.

Primary targets:

- RV64 inline asm tests that already cover `.insn.d` object emission.
- Any frontend or integration test harness that compiles inline asm through the
  folded-template path.

Actions:

- Add a positive case for compile-time concatenation of `.insn.d` template
  fragments.
- Add a positive helper-style case, such as a consteval or template fixed-string
  helper, only using the selected representation from Step 1.
- Compare emitted bytes with the literal `.insn.d` route, including the known
  EV byte sequence when applicable.
- Keep `VRM2` operand constraints and positional placeholders in the proof.

Completion check:

- The `.insn.d` helper test emits exactly the expected 8-byte instruction
  through c4c's object route.
- The literal and compile-time-built forms remain equivalent in final template
  text and emitted bytes.

## Step 4: Broaden Proof And Prepare Closure

Goal: Establish that the feature is acceptance-ready without regressing the
completed child stages.

Actions:

- Run the focused build and tests touched by parser/sema/HIR/LIR/backend work.
- Run the supervisor-selected broader subset because this feature may cross
  frontend and backend boundaries.
- Check that existing `.insn`, vector constraint, and `.insn.d` tests were not
  weakened or reclassified.
- Summarize proof and any known leftovers in `todo.md`.

Completion check:

- Focused positive and negative tests pass.
- Broader validation has no new failure beyond supervisor-accepted baseline
  failures.
- The child source idea is ready for plan-owner close review.
