Status: Active
Source Idea Path: ideas/open/267_aarch64_inline_asm_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Inline-Asm Shard And Current Route

# Current Packet

## Just Finished

Lifecycle activation created this active packet skeleton for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: audit the inline-asm markdown shard and current
AArch64 route, then record the current/deferred/obsolete facts here before any
code movement.

## Watchouts

- Keep this route behavior-preserving unless current prepared facts and tests
  already authorize the behavior.
- Do not port the legacy inline-asm substitution machinery wholesale.
- Do not mix atomic, memory, allocation, assembler, or unrelated printer
  rewrites into this shard redistribution.

## Proof

No build proof required for lifecycle-only activation.
