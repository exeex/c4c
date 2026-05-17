Status: Active
Source Idea Path: ideas/open/268_aarch64_intrinsics_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Intrinsics Shard And Current Route

# Current Packet

## Just Finished

Lifecycle activation created this active packet skeleton for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: audit the intrinsics markdown shard and current
AArch64 route, then record the current/deferred/obsolete facts here before any
code movement.

## Watchouts

- Keep this route behavior-preserving unless current prepared facts and tests
  already authorize the behavior.
- Do not port the legacy NEON, CRC, cache, hint, address builtin, or soft-float
  helper surface wholesale.
- Do not mix scalar float, F128, memory, call, return, allocation, assembler,
  or unrelated printer rewrites into this shard redistribution.

## Proof

No build proof required for lifecycle-only activation.
