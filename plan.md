# Plan: C++ dependent reference alias C-style cast lowering

Status: Active
Source Idea: ideas/open/283_cpp_dependent_reference_alias_c_style_cast_lowering.md

## Purpose

Repair C-style cast lowering for dependent lvalue/rvalue reference aliases so
casts keep scalar/reference semantics instead of producing aggregate-shaped IR
stores.

Goal: make the three dependent reference alias C-style cast runtime tests pass
without weakening overload selection, test expectations, or template alias
semantics.

Core Rule: fix the semantic type/value-category boundary for dependent
reference aliases; do not special-case filenames, `Holder<T>::Rebind`, or the
known generated IR shape.

## Read First

- `ideas/open/283_cpp_dependent_reference_alias_c_style_cast_lowering.md`
- The three failing tests named in the source idea.
- The HIR/BIR/LLVM lowering paths for C-style casts, dependent aliases, and
  reference materialization.

## Current Targets

- `cpp_positive_sema_c_style_cast_dependent_template_member_ref_alias_basic_cpp`
- `cpp_positive_sema_c_style_cast_nested_dependent_template_member_ref_alias_basic_cpp`
- `cpp_positive_sema_c_style_cast_nested_dependent_typename_ref_alias_basic_cpp`

## Non-Goals

- Do not weaken, skip, or reclassify the three positive runtime tests.
- Do not add testcase-shaped handling for the exact filenames or holder
  template spelling.
- Do not fold in default-template return cast repair, C aggregate call ABI, or
  AArch64 fp128/vararg crash work.
- Do not rewrite broad template substitution or overload resolution machinery
  unless the focused boundary proves it is required.

## Working Model

- The source failures cast through dependent aliases that resolve to `U&` and
  `U&&`.
- The bad IR indicates the resolved scalar/reference value is being treated as
  an aggregate holder type during cast/reference lowering.
- The fix should preserve ref-overload behavior: lvalue casts select `int&`
  overloads and rvalue casts select `int&&` overloads.

## Execution Rules

- Keep each implementation packet tied to one observable boundary.
- Record packet progress and proof in `todo.md`; do not rewrite this runbook
  for routine executor progress.
- For code-changing steps, run a fresh build before CTest proof.
- A narrow green result is not sufficient if the diff weakens expectations,
  bypasses overload resolution, or adds named-case matching.

## Steps

### Step 1: Locate The Reference Alias Cast Boundary

Goal: identify where the resolved reference alias loses scalar/reference
semantics and becomes aggregate-shaped.

Primary targets:
- the three failing tests
- C-style cast lowering
- dependent nested template alias substitution
- reference materialization and assignment lowering

Actions:
- Reproduce at least one failing target and capture the invalid HIR/BIR/LLVM
  evidence.
- Compare direct, nested template-member, and nested typename alias forms.
- Identify the first lowering layer where the cast target or result type stops
  carrying the resolved `U&` / `U&&` semantics.
- Record the suspected source files/functions and the narrow proof command in
  `todo.md`.

Completion check:
- `todo.md` names the first bad boundary and includes concrete failing IR or
  dump evidence showing why the next packet can make a focused repair.

### Step 2: Repair Dependent Reference Alias Cast Semantics

Goal: make C-style casts to dependent reference aliases lower through the
resolved reference/scalar type rather than the dependent holder aggregate.

Actions:
- Update the focused cast/type substitution boundary found in Step 1.
- Preserve structured dependent alias information long enough for reference
  cast lowering to see the resolved lvalue/rvalue reference target.
- Keep overload selection behavior unchanged except for correcting the bad
  alias/reference semantics.
- Avoid filename, test-name, and holder-template checks.

Completion check:
- The repaired HIR/BIR/LLVM evidence for at least one target shows a
  scalar/reference cast path and no `i32` stored as `%struct.Holder_T_int`.
- The focused target passes after a fresh build.

### Step 3: Prove The Three Alias Forms

Goal: verify the repair is general across direct, nested template-member, and
nested typename reference alias forms.

Actions:
- Run the focused CTest regex covering all three source-idea targets.
- Inspect enough generated evidence to confirm both lvalue and rvalue alias
  casts preserve the expected overload behavior.
- If one form still fails, keep the repair at the shared alias/reference
  lowering boundary rather than adding form-specific shortcuts.

Completion check:
- All three `cpp_positive_sema_c_style_cast_*dependent*alias*` tests pass under
  CTest.
- `todo.md` records the passing command and the overload/reference evidence.

### Step 4: Broaden Reference, Cast, And Template Validation

Goal: ensure the focused repair does not regress nearby reference, cast, or
template behavior.

Actions:
- Run nearby positive reference, cast, and template test subsets selected by
  the supervisor.
- Escalate to a broader CTest or regression-guard check if the diff touched
  shared substitution, cast, overload, or reference materialization code.
- Record any remaining unrelated failures separately instead of folding them
  into this idea.

Completion check:
- Focused targets remain green.
- Nearby validation is green, or any failure is clearly unrelated and
  documented with evidence.
- No expectation downgrade, testcase skip, or harness weakening is present.
