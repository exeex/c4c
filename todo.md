# AArch64 Scalar Parameter ALU Authority Todo

Status: Active
Source Idea Path: ideas/open/290_aarch64_scalar_parameter_alu_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Scalar Parameter ALU Failure

# Current Packet

## Just Finished

Activated `ideas/open/290_aarch64_scalar_parameter_alu_authority.md` as the
single active plan.

## Suggested Next

Delegate Step 1 to establish and localize the `src/00124.c` scalar
parameter/ALU authority failure before implementation begins.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Do not reopen the closed function-pointer materialization owner unless new
  proof contradicts the closure review.
- Preserve the existing valid function-pointer return and indirect `blr`
  shape.
- Reject named-case shortcuts for `src/00124.c`, one function name, one
  parameter name, or one arithmetic expression.

## Proof

No execution proof was run during activation. The next executor packet should
capture the Step 1 baseline with the supervisor-selected command.
