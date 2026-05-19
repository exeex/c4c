# AArch64 Scalar Machine Node Operand Forms Runbook

Status: Active
Source Idea: ideas/open/302_aarch64_scalar_machine_node_operand_forms.md
Supersedes: ideas/open/295_backend_regex_failure_family_inventory.md active umbrella route for this focused owner

## Purpose

Repair the AArch64 backend path where scalar arithmetic and reduction machine
nodes reach assembly printing without structured operands the printer accepts.

## Goal

Make the focused scalar `div`, scalar `mul`, and scalar
`logical_shift_right` unsigned-reduction cases print through valid AArch64
operand forms without testcase-specific shortcuts.

## Core Rule

Treat this as a semantic scalar machine-node operand-form repair. Do not
change expectations, allowlists, unsupported classifications, timeout policy,
runner behavior, or CTest registration to claim progress.

## Read First

- Source idea: `ideas/open/302_aarch64_scalar_machine_node_operand_forms.md`
- Split source: `ideas/open/295_backend_regex_failure_family_inventory.md`
- Accepted broad proof baseline: `test_before.log`
- Focused c-testsuite cases: `00064`, `00139`, `00205`
- Closed owner boundaries to respect: ideas 296, 299, 300, and 301

## Current Targets

- Scalar `div` operand forms in `c_testsuite_aarch64_backend_src_00064_c`.
- Scalar `mul` operand forms in `c_testsuite_aarch64_backend_src_00139_c`.
- Scalar `logical_shift_right` unsigned-reduction operand forms in
  `c_testsuite_aarch64_backend_src_00205_c`.

## Non-Goals

- Do not merge assembly legality/materialization singletons `00104` or `00182`
  into this owner without generated-code evidence.
- Do not merge call-boundary move `00140` into this owner.
- Do not merge `lir_to_bir` residuals `00204` or `00216` into this owner.
- Do not merge runtime nonzero, runtime mismatch/crash, timeout, output-storm,
  libc, floating, ABI, aggregate, pointer, string, or control-flow buckets into
  this owner.
- Do not reopen closed owners 285 through 301 from counts alone.
- Do not match only c-testsuite filenames or exact diagnostic strings.

## Working Model

- The focused failures are compile-stage machine-node operand-form failures,
  not runtime-output classification.
- The repair should ensure selected scalar arithmetic/reduction nodes publish
  operands in the structured forms required by the AArch64 printer.
- A valid slice may improve one form at a time, but acceptance for this owner
  needs the full focused three-case subset classified and proved.

## Execution Rules

- Start each implementation packet by inspecting the current diagnostics or
  generated machine-node route for the delegated focused case set.
- Prefer semantic lowering, selection, operand publication, or materialization
  over printer-only string handling.
- Keep changes scoped to scalar arithmetic/reduction operand forms unless the
  generated-code evidence proves a shared helper boundary.
- Record fresh build proof and the focused c-testsuite subset in `todo.md`
  before asking for broader backend-regex acceptance.
- Report remaining residual buckets separately from this owner.

## Steps

### Step 1: Inspect Focused Operand Diagnostics

Goal: identify exactly which scalar machine-node operands reach printing in an
unsupported form.

Primary target: focused backend c-testsuite cases `00064`, `00139`, and
`00205`

Actions:

- Run or inspect the focused cases enough to capture the active diagnostics.
- Trace the selected scalar `div`, `mul`, and `logical_shift_right` nodes to
  the operand forms handed to the AArch64 printer.
- Identify whether each failure should be repaired by selection,
  materialization, operand publication, or printer admission.

Completion check:

- `todo.md` records the active diagnostic for each focused case and names the
  shared implementation surface or the reason the packet must split.

### Step 2: Repair Scalar Arithmetic Operand Forms

Goal: publish or materialize printable operands for the scalar arithmetic
forms without testcase-shaped matching.

Primary target: scalar `div` and scalar `mul` operand routes

Actions:

- Repair the semantic path that leaves scalar arithmetic operands
  unstructured.
- Keep the implementation general for the affected scalar node forms.
- Build and prove the relevant focused subset selected by the supervisor.

Completion check:

- `00064` and `00139` no longer fail from the old scalar arithmetic
  operand-form diagnostics, and no expectation or runner contract changed.

### Step 3: Repair Unsigned-Reduction Shift Operand Forms

Goal: publish or materialize printable operands for the scalar
`logical_shift_right` unsigned-reduction route.

Primary target: focused case `00205`

Actions:

- Trace the unsigned-reduction path to the unsupported shift operand.
- Repair the shared operand-form handling without absorbing unrelated runtime
  or `lir_to_bir` buckets.
- Build and prove the focused subset selected by the supervisor.

Completion check:

- `00205` no longer fails from the old scalar
  `logical_shift_right`/unsigned-reduction operand-form diagnostic, and the
  focused subset has fresh proof.

### Step 4: Focused Closure Proof

Goal: show the focused scalar operand-form owner is complete and ready for
broader supervisor validation.

Primary target: focused c-testsuite subset plus supervisor-selected broader
backend proof

Actions:

- Rebuild after the implementation slice.
- Run the focused three-case backend c-testsuite subset.
- Run broader backend-regex proof only when the supervisor selects it.
- Summarize remaining residuals separately from this owner.

Completion check:

- The old focused scalar machine-node operand-form diagnostics are absent from
  `00064`, `00139`, and `00205`; proof is recorded in `todo.md`; no excluded
  bucket is claimed as solved without evidence.
