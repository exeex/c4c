Status: Active
Source Idea Path: ideas/open/645_rv64_branch_residual_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Residual Terminator Evidence

# Current Packet

## Just Finished

Lifecycle activated Step 1 of `plan.md` for
`ideas/open/645_rv64_branch_residual_terminator_fragment_lowering.md`.

## Suggested Next

Execute Step 1: refresh diagnostics for the eight representative branch
residual rows and group them by concrete BIR terminator shape plus first
unsupported lowering boundary.

## Watchouts

- Do not publish new branch freshness or clobber-safety authority in this
  idea.
- Do not accept stack branch operands from stack offsets, frame homes, final
  assembly shape, or assumed no-clobber behavior.
- Do not add named-case handling for the representative source files.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or pass/fail accounting as capability progress.

## Proof

No code proof is required for this lifecycle activation. The previous idea 652
close gate passed with matching focused logs:
`passed=8 failed=0 total=8` before and after.
