# AArch64 C-Testsuite Undefined Temporary Labels

Status: Closed
Created: 2026-05-18
Closed: 2026-05-18

## Intent

Repair the AArch64 backend failure family where c-testsuite backend-route
cases emit references to `.LBB...` temporary labels that are not defined in the
generated assembly.

## Why This Exists

The completed call-boundary move lowering route removed the old owned
`deferred_unsupported` diagnostic from the broad AArch64 backend scan. The
same scan then exposed a separate non-runtime backend family: cases such as
`00005.c`, `00006.c`, `00040.c`, `00156.c`, and `00220.c` fail with undefined
temporary label references. This is distinct from runtime-runner availability
and from call-boundary move lowering.

## In Scope

- Identify representative AArch64 c-testsuite cases that reference undefined
  `.LBB...` temporary labels.
- Trace whether the missing definitions originate in BIR/LIR block labeling,
  prepared handoff, AArch64 branch lowering, or assembly emission.
- Implement a general label-authority or emission repair for the missing
  temporary label definitions.
- Preserve stage-specific diagnostics for frontend, backend, assembler/link,
  runtime unavailable, runtime nonzero, and runtime mismatch outcomes.
- Prove the fix with focused backend tests and the relevant AArch64 c-testsuite
  backend subset.

## Out of Scope

- Configuring an AArch64 runtime runner or claiming runtime pass evidence.
- Reopening the completed call-boundary move lowering route unless the exact
  old diagnostic returns.
- Weakening c-testsuite expected outputs, allowlists, unsupported
  classifications, or stage-specific diagnostics.
- Adding filename-specific label definitions or case-name shortcuts.
- Broad rewrites of unrelated AArch64 ABI, call lowering, or frontend paths.

## Acceptance Criteria

- Representative undefined temporary label failures no longer emit references
  to labels that are absent from the generated assembly.
- The fix is expressed as a general block-label, branch-target, or assembly
  emission rule rather than c-testsuite filename matching.
- Focused backend proof demonstrates the repaired label-definition path.
- The AArch64 c-testsuite backend subset shows the targeted failure family is
  reduced or reclassified to the next truthful owner layer without counting
  `[RUNTIME_UNAVAILABLE]` as pass evidence.

## Completion Notes

The active runbook completed through Step 6. Focused backend proof demonstrated
the repaired label-definition path, and the broad `aarch64_backend` scan
reported zero remaining undefined temporary `.LBB...` diagnostics.

The representative cases from the source idea (`00005.c`, `00006.c`, `00040.c`,
`00156.c`, and `00220.c`) moved past the undefined temporary-label blocker and
now classify as `[RUNTIME_UNAVAILABLE]` when reached in this environment.
Runtime-unavailable was not counted as pass evidence.

The same broad scan exposed a separate backend-symbol family in `00207.c`
involving undefined LLVM stack helpers. That work is tracked separately in
`ideas/open/279_aarch64_c_testsuite_llvm_stack_helper_symbols.md`.

## Reviewer Reject Signals

Reject the route or slice if it:

- matches c-testsuite filenames, exact case names, or exact `.LBB` spellings
  instead of repairing label authority or emission semantics;
- only rewrites expectations, allowlists, unsupported classifications, or
  diagnostics while undefined temporary labels remain in generated assembly;
- claims runtime capability progress without an AArch64 runner or native host;
- collapses backend, assembler/link, and runtime failures into one generic
  failure bucket;
- fixes one undefined label by hard-coding a synthetic definition without
  proving nearby branch or block-label cases;
- hides missing frontend/BIR facts inside target-specific printer code.
