Status: Active
Source Idea Path: ideas/open/665_aarch64_instruction_dispatch_internal.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh AArch64 Dispatch And Publication Evidence

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for idea 665. No execution
packet has run yet.

## Suggested Next

Delegate Step 1 to an executor: refresh focused evidence for
`backend_aarch64_instruction_dispatch` and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`,
then classify whether row 322 shares the AArch64 owner or should split back to
a generic prepared/CLI contract.

## Watchouts

- Keep RV64 runtime, RISC-V object emission, byval, stack fan-in, and LLVM
  torture rows out of this route.
- Do not treat expectation edits, unsupported-marker changes, allowlist edits,
  timeout changes, runtime policy changes, or baseline accounting as progress.
- Reject testcase-shaped dispatch-table shortcuts and CLI-format-only claims
  while AArch64 facts or dispatch coverage are missing.

## Proof

Lifecycle-only activation; no build or test proof required.
