# AArch64 Call Argument Register Authority Todo

Status: Active
Source Idea Path: ideas/open/291_aarch64_call_argument_register_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Call Argument Register Authority Failure

# Current Packet

## Just Finished

Activated `ideas/open/291_aarch64_call_argument_register_authority.md` as the
single active plan.

## Suggested Next

Delegate Step 1 to establish and localize the `src/00210.c` call-argument
register-authority failure before implementation begins.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Do not reopen the closed function-pointer materialization owner unless new
  proof contradicts the closure review.
- Do not reopen the closed scalar parameter/ALU owner unless new proof
  contradicts that closure review.
- Preserve existing attributed function-pointer indirect calls through
  `actual_function`.
- Reject named-case shortcuts for `src/00210.c`, `.str0`, `printf`, one
  symbol, or one emitted move shape.

## Proof

No execution proof was run during activation. The next executor packet should
capture the Step 1 baseline with the supervisor-selected command.
