Status: Active
Source Idea Path: ideas/open/278_aarch64_c_testsuite_undefined_temporary_labels.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Identify Representative Undefined-Label Cases

# Current Packet

## Just Finished

Activation created the runbook for Step 1 from `plan.md`. No executor packet
has run yet.

## Suggested Next

Run Step 1: identify representative AArch64 backend c-testsuite cases that
reference undefined `.LBB...` temporary labels, record the narrow proof command,
and separate this family from runtime-unavailable, scalar printer, semantic
`lir_to_bir`, and unrelated backend failures.

## Watchouts

- Do not re-activate the parked runtime-runner route from
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md` unless an
  AArch64 host or `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is available.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- Do not match c-testsuite filenames, exact case names, or exact `.LBB`
  spellings instead of repairing label authority or emission semantics.

## Proof

Lifecycle-only activation; no build or test proof required.
