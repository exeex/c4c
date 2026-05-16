Status: Active
Source Idea Path: ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Calls Ownership Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md`.

## Suggested Next

Delegate Step 1 to an executor: inspect the calls markdown shard and broad
AArch64 instruction, dispatch, and machine-printer surfaces to identify
concrete behavior-preserving extraction targets for `calls.cpp` and
`calls.hpp`.

## Watchouts

- Do not redistribute other AArch64 markdown shards in this route.
- Preserve call behavior; do not downgrade tests or weaken supported-path
  contracts to claim progress.
- Keep ABI authority in prepared call plans, `module::CallRecord`,
  `module::MoveRecord`, `module::AbiBindingRecord`, allocation-result records,
  and the AAPCS64 call/return contract.
- Keep broad owners family-neutral and route call-family bodies into
  `calls.cpp`/`calls.hpp`.
- Do not delete `calls.md` until its durable behavior and risks are reconciled
  into compiled calls ownership.

## Proof

Lifecycle-only activation; no code validation run.
