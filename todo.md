Status: Active
Source Idea Path: ideas/open/251_aarch64_alu_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect ALU Ownership Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/251_aarch64_alu_markdown_shard_implementation_redistribution.md`.

## Suggested Next

Delegate Step 1 to an executor: inspect the ALU markdown shard, compiled ALU
owners, and broad AArch64 instruction, dispatch, and machine-printer surfaces
to identify concrete behavior-preserving extraction targets.

## Watchouts

- Do not redistribute other AArch64 markdown shards in this route.
- Preserve ALU behavior; do not downgrade tests or weaken supported-path
  contracts to claim progress.
- Keep broad owners family-neutral and route ALU-family bodies into
  `alu.cpp`/`alu.hpp`.
- Do not delete `alu.md` until its durable behavior and risks are reconciled
  into compiled ALU ownership.

## Proof

Lifecycle-only activation; no code validation run.
