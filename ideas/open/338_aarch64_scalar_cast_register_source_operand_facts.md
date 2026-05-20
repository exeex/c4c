# AArch64 Scalar Cast Register-Source Operand Facts

Status: Open
Created: 2026-05-20
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the AArch64 scalar cast machine-printer path where selected
`sign_extend` and `zero_extend` machine nodes reach printing without a
structured register source operand.

## Why This Exists

The post-337 backend-regex classification found a focused 9-case external
AArch64 residual bucket: `00143`, `00173`, `00175`, `00176`, `00181`,
`00185`, `00204`, `00205`, and `00216`.

Eight representatives fail at the AArch64 machine printer with
`opcode=sign_extend` and the diagnostic that a scalar cast node requires a
structured register source operand. `00216` reaches the same printer boundary
with `opcode=zero_extend`.

The local/internal backend regex split is currently clean. The current first
bad fact is compile-stage selected-node operand publication/printing, not
runtime behavior. This is adjacent to closed scalar operand-fact owners such
as idea 334, but it is distinct from scalar `mul`/`add` RHS or frame-slot
printing because the operation owner here is scalar cast register-source fact
publication for sign/zero extension.

## In Scope

- Localize where selected scalar cast nodes lose or fail to publish their
  register source operand before AArch64 printing.
- Repair structured source operand publication or normalization for selected
  `sign_extend` and `zero_extend` register-sourced cast nodes.
- Preserve the selected-node contract so the AArch64 printer receives a
  printable register operand rather than an unstructured or missing source.
- Add or update focused backend coverage for scalar cast selected-node operand
  facts without depending only on c-testsuite filenames.
- Prove the 9-case c-testsuite bucket advances past the current
  machine-printer diagnostic or passes.

## Out Of Scope

- Reopening closed ideas 334 through 337 from pass/fail counts alone.
- Reopening the earlier scalar arithmetic operand-fact owner unless generated
  artifacts show the current first bad fact is the same scalar `mul`/`add`
  boundary.
- Reopening runtime nonzero, runtime mismatch, timeout/output-storm,
  aggregate, pointer, vararg, frame-layout, or semantic `lir_to_bir` owners.
- Changing expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Fixing only one c-testsuite file, one opcode spelling, one emitted
  diagnostic string, one source line, one register number, or one selected
  instruction name.

## Acceptance Criteria

- The current `sign_extend` and `zero_extend` printer diagnostics are
  localized to a concrete scalar cast operand-fact producer, normalizer, or
  selected-node handoff boundary.
- Focused backend coverage proves selected scalar cast nodes carry structured
  register source operands into the AArch64 printer.
- The focused 9-case bucket no longer fails with the structured register
  source operand diagnostic.
- Any remaining failures in the 9 cases are reclassified by their new first
  bad fact before this idea is closed.
- No expectation, runner, timeout, unsupported, or CTest-registration change is
  used to claim progress.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00143`, `00173`, `00175`, `00176`, `00181`, `00185`,
  `00204`, `00205`, `00216`, one cast opcode, one diagnostic string, one
  register, one source line, or one emitted instruction sequence instead of
  repairing scalar cast register-source operand facts generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to reduce the failure count;
- claims progress only through helper renames, diagnostic rewrites,
  classification notes, or c-testsuite expectation changes;
- broadens into runtime value correctness, frame layout, variadic handling,
  aggregate projection, pointer materialization, or semantic `lir_to_bir`
  admission without fresh evidence that the scalar cast first bad fact has
  moved there;
- reopens closed ideas 334 through 337, or earlier scalar arithmetic/cast
  owners, without generated-code or proof evidence tying the current first bad
  fact to the exact closed boundary;
- leaves selected scalar cast nodes reaching AArch64 printing without a
  structured register source operand behind a renamed abstraction.
