Status: Active
Source Idea Path: ideas/open/611_rv64_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Terminator-Fragment Residual Diagnostics

# Current Packet

## Just Finished

Lifecycle activation initialized idea 611 into `plan.md` and reset execution
state for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: refresh the current RV64
`unsupported_terminator_fragment` residual diagnostics, split consumer-owned
rows from authority-owned rows, and record the exact Step 2 implementation
packet.

## Watchouts

- Do not infer missing branch operands from final layout.
- Keep branch stack-source freshness and prepared authority publication out of
  scope.
- Do not touch expectations, unsupported markers, allowlists, runtime,
  timeout/accounting, move-bundle, ABI, or generic instruction-fragment work.

## Proof

Lifecycle-only activation. No code validation run.
