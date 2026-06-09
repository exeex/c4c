Status: Active
Source Idea Path: ideas/open/146_call_argument_materialization_call_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current Call-Argument Materialization Ownership

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Delegate Step 1 to an executor: map the current call-argument materialization
declarations, definitions, consumers, and same-block/source-producer dependency
before moving ownership.

## Watchouts

- Keep this route limited to call-argument materialization ownership.
- Do not duplicate same-block materialization logic in call ownership.
- Do not turn the lookup into an AArch64-only shortcut.
- Do not mix ABI move-bundle work into this packet.

## Proof

Lifecycle-only activation; no build or test proof required.
