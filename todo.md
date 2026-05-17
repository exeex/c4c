Status: Active
Source Idea Path: ideas/open/269_aarch64_peephole_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Or Record Integration

# Current Packet

## Just Finished

Completed plan Step 3, "Route Or Record Integration", by recording the current
integration decision for the deferred AArch64 peephole boundary without changing
output behavior.

Current integration is intentionally not wired into the assembly output path
because there is no active AArch64 peephole pass and no accepted
printer-output cleanup policy hook. The compiled `peephole.{hpp,cpp}` pair is
the discoverable owner boundary for this deferred behavior; broader owners such
as emitters, printers, module compilation, or backend driver code must not hide
peephole behavior.

## Suggested Next

Execute the next supervisor-selected packet for plan Step 4, likely deleting
the obsolete `src/backend/mir/aarch64/codegen/peephole.md` shard after the
compiled deferred boundary has been preserved.

## Watchouts

Do not wire `apply_deferred_peephole_boundary` just to exercise the boundary.
Any future integration needs an explicit accepted output-policy hook or a real
AArch64 peephole pass. Do not reintroduce the legacy text optimizer,
RISC-V-style `peephole_optimize` surface, string classifier, global copy
propagation, or dead stack-store passes.

## Proof

No build required for this todo-only packet. No proof command was run, and
`test_after.log` was intentionally not changed.
