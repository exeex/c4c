Status: Active
Source Idea Path: ideas/open/623_rv64_cast_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh cast residual evidence

# Current Packet

## Just Finished

Activation created the runbook from
`ideas/open/623_rv64_cast_instruction_fragment_consumers.md`; no implementation
work has started.

## Suggested Next

Execute Step 1 from `plan.md`: refresh cast-shaped
`unsupported_instruction_fragment` diagnostics, record the refresh command and
proof artifact, and separate cast rows from nearby non-cast guard rows before
any implementation work.

## Watchouts

- Do not implement from stale idea-612 counts.
- Do not broaden into non-cast pointer, ABI, local-memory, select, branch,
  move-bundle, global, runtime, terminator, expectation, unsupported-marker,
  allowlist, timeout, or accounting changes.
- Reject named-case-only cast lowering.

## Proof

Lifecycle activation only; no build or test proof required.
