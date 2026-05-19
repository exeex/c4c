Status: Active
Source Idea Path: ideas/open/318_aarch64_scalar_alu_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar ALU Immediate Path

# Current Packet

## Just Finished

Lifecycle switch complete. Idea 317 is closed after Step 4 classified the
remaining `00204.c` residual as outside variadic `va_start` helper lowering.
Idea 318 is now active for scalar ALU immediate materialization.

## Suggested Next

Start Step 1 - Localize Scalar ALU Immediate Path. Identify the scalar ALU
code surface that emits illegal non-encodable constants such as
`mov w9, #503808`, then record the owner, tests, and exact focused proof
command here before implementation.

## Watchouts

- Do not reopen idea 317's raw `va_start` helper-text lowering owner.
- Do not reopen idea 315's large frame setup/teardown materialization owner.
- Do not reopen idea 314's stack-slot memory or scalar stack-publication owner.
- Do not fold idea 316's frame-slot/frame-layout consistency residual into
  this owner.
- Do not special-case `00204.c`, `subim503808`, literal `503808`, one
  scratch register, one diagnostic, or one emitted line.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, CTest registration, proof-log policy, or prepared handoff contracts.

## Proof

Close-time guard for idea 317 used existing focused `test_before.log` and
`test_after.log` with non-decreasing lifecycle close mode. Both logs report
10/11 passing, with no new failures. The only focused failure is
`c_testsuite_aarch64_backend_src_00204_c` at `mov w9, #503808`, which is the
active idea 318 residual.
