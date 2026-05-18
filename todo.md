Status: Active
Source Idea Path: ideas/open/277_aarch64_c_testsuite_call_boundary_move_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Identify Representative Failure Cases

# Current Packet

## Just Finished

Activated Step 1 from `plan.md`; no implementation work has been done under
this runbook yet.

## Suggested Next

Execute Step 1: identify representative AArch64 backend c-testsuite failures
where call-boundary move lowering or selection reaches the machine-node printer
as `deferred_unsupported`, then record the narrow proof command for the first
implementation packet.

## Watchouts

- Do not re-activate the parked runtime-runner route from
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md` unless an
  AArch64 host or `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is available.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.

## Proof

Lifecycle activation only. No build or test proof is required for this packet.
