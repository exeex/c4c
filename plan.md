# C++ Dependent Template Cast Return IR Type Repair

Status: Active
Source Idea: ideas/open/282_cpp_dependent_template_cast_return_ir_type_repair.md

## Purpose

Repair dependent-template cast and return-type materialization so substituted
`static_cast<T>(value)` return expressions lower to valid LLVM IR with the
required scalar conversion.

## Goal

Make `convert<long, short>(short val)` and nearby dependent-template cast
returns carry resolved target and return types through HIR/LIR lowering so the
backend emits a valid widened return instead of returning an `i16` value as
`i32`.

## Core Rule

Fix the general dependent-template cast/return type path. Do not special-case
`template_default_args.cpp`, `convert_l_s`, or hide unresolved `?` types in the
IR printer.

## Read First

- `ideas/open/282_cpp_dependent_template_cast_return_ir_type_repair.md`
- `tests/cpp/positive/sema/template_default_args.cpp`
- Template materialization and substitution code for function return types,
  dependent cast target types, `CastExpr`, and `ReturnStmt`.
- HIR-to-LIR return and scalar cast lowering code before deciding the repair
  boundary.

## Current Scope

- Inspect why materialized HIR shows `convert_l_s(val: short) -> ?` and
  `((?)val#P0)`.
- Repair the substitution or lowering boundary that loses the dependent cast
  target and return type.
- Ensure LIR return coercion emits the correct scalar conversion such as
  `sext`, `zext`, or `trunc` after substitution.
- Preserve existing non-template casts, default template arguments, NTTP
  materialization, and positive runtime behavior.

## Non-Goals

- Do not weaken positive-case runtime tests or mark the target case
  unsupported.
- Do not rewrite unrelated template instantiation machinery unless the direct
  substitution boundary requires it.
- Do not paper over invalid IR in the printer.
- Do not fold in reference alias C-style cast repair, C aggregate function
  pointer ABI work, or AArch64 fp128/vararg crash triage.

## Working Model

The failing IR indicates the materialized specialization has the parameter type
resolved to `short` but has lost either the function return type, the
`static_cast<T>` target type, or both before return lowering. The first packet
should identify the exact point where `T` remains unresolved as `?`. Later
packets should repair that boundary and prove that return lowering sees a
fully substituted scalar source and destination type.

## Execution Rules

- Prefer semantic substitution and lowering repairs over testcase-shaped
  matching.
- Keep diagnostics and dumps useful; unresolved HIR/template type state should
  be fixed, not hidden.
- Preserve existing behavior for non-template casts and other template
  materialization paths.
- Keep changes narrow to the dependent cast/return materialization path unless
  inspection proves the bug lives in a shared substitution helper.
- Run the supervisor-delegated proof command exactly when provided.
- Treat expectation rewrites, runtime skips, harness weakening, or IR-printer
  hacks as route drift.

## Step 1: Locate The Unresolved Type Boundary

Goal: identify where the materialized specialization loses the dependent
return type or `static_cast<T>` target type.

Actions:

- Reproduce the target failure and inspect `--dump-hir` / relevant debug dumps
  for `convert<long, short>`.
- Trace template substitution for function return types and dependent
  `CastExpr` target types.
- Trace `ReturnStmt` lowering far enough to see whether it receives unresolved
  HIR type state or loses resolved type information later.
- Record the exact owning helper or boundary that should be repaired in the
  next step.

Completion check:

- `todo.md` names the unresolved type boundary, the examined dumps, and the
  proposed Step 2 repair target without changing implementation behavior.

## Step 2: Repair Dependent Cast And Return Substitution

Goal: ensure materialized dependent casts and function return types carry
resolved scalar types into lowering.

Actions:

- Update the substitution/materialization boundary found in Step 1 so
  `static_cast<T>(value)` records the substituted target type.
- Ensure the materialized function return type is resolved before LIR return
  coercion.
- Keep the repair general across dependent-template casts and default template
  argument instantiations.
- Avoid broad template rewrites unless a shared substitution helper is the
  smallest correct repair point.

Completion check:

- `--dump-hir` for the target specialization no longer renders the return type
  or cast target as `?`, and no nearby template materialization behavior is
  intentionally changed.

## Step 3: Prove Scalar Return Coercion

Goal: verify LIR/LLVM lowering emits a valid scalar conversion for substituted
return types.

Actions:

- Inspect generated IR for `convert<long, short>`.
- Confirm the widened return path emits the required conversion before
  returning the value.
- Check nearby dependent casts for signed, unsigned, narrowing, or same-width
  scalar behavior as applicable.
- Add or adjust targeted coverage only if existing tests do not prove the
  repaired general path.

Completion check:

- Generated IR returns a value of the declared return type and does not contain
  `ret i32 %p.val` or equivalent invalid source/destination mismatch.

## Step 4: Focused Acceptance Proof

Goal: prove the target failure and nearby dependent-template paths remain
green.

Actions:

- Run:
  `ctest --test-dir build_debug -R '^cpp_positive_sema_template_default_args_cpp$' --output-on-failure`
- Run the supervisor-selected nearby dependent template cast/default argument
  subset if provided.
- Preserve `test_after.log` as the executor proof log when tests are run.

Completion check:

- The target CTest command passes, `todo.md` records the command and result,
  and any nearby subset requested by the supervisor is also green or has a
  documented pre-existing unrelated failure.

## Step 5: Closeout Notes

Goal: leave enough evidence for the plan owner to decide whether idea 282 is
complete.

Actions:

- Record the substitution/lowering boundary that was repaired.
- Record the before/after HIR and IR facts for the target specialization.
- Confirm non-template casts, default template arguments, NTTP materialization,
  and runtime positive-case behavior were not weakened.
- Confirm no reference alias, C aggregate ABI, or AArch64 fp128/vararg scope
  was folded into this runbook.

Completion check:

- `todo.md` contains concise closeout evidence that maps back to the source
  idea acceptance criteria and reviewer reject signals.
