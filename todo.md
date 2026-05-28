Status: Active
Source Idea Path: ideas/open/66_aarch64_local_dispatch_block_route.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Keep Diagnostics Durable While Narrowing Route Helpers

# Current Packet

## Just Finished

Step 3 completed: preserved durable AArch64 dispatch diagnostics and justified
the diagnostic helpers as route-local without implementation edits.

Diagnostic helper decision:
- `classify_instruction` remains a private route-local helper because it maps
  BIR variants to AArch64 dispatch diagnostic families for unsupported
  instruction reporting.
- `append_block_diagnostic` remains private and route-local because it preserves
  block-route context for missing prepared function/block context and missing
  retained-BIR block mapping diagnostics.
- `unsupported_terminator_message` remains private and route-local because
  terminator diagnostics are AArch64 block-route messages and must keep their
  current text.
- `append_unsupported_instruction_diagnostic` remains private and route-local
  because it preserves diagnostic kind, function/block context, instruction
  index, instruction family, and the existing unsupported-instruction message.
- No behavior-preserving boundary cleanup was clearer than leaving these
  helpers private in `dispatch.cpp`; no diagnostic text, kind, context, index,
  or family assignment was changed.

## Suggested Next

Step 4 should preserve hook wiring and sequencing boundaries. Suggested packet:
inventory and justify `make_dispatch_branch_fusion_hooks`,
before-instruction move ordering, before-return publication sequencing,
branch-fusion integration, prepared-memory retry routing, and final
instruction append order; narrow only if ordering remains visibly local.

## Watchouts

This plan preserves the local AArch64 block route. Do not move block traversal
or target diagnostics to shared code, do not reintroduce producer/publication
or value-materialization authority into `dispatch.cpp`, and do not bundle idea
67's local-slot offset probe into this route.

Proof coverage needed for Step 4: before-return publication, branch-fusion
hooks, prepared-memory retry routing, direct `dispatch_prepared_block`
sequencing, production traversal through `traversal.cpp`, and representative
prepared block lowering. Do not move hook sequencing or before-return ordering
out of the local route.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_|backend_cli_aarch64_)') > test_after.log 2>&1`

`test_after.log` reports 32/32 tests passed. `git diff --check` passed.
Targeted `rg` scan confirmed diagnostic helpers are private to `dispatch.cpp`
and used only by route-local missing-context, missing-BIR mapping,
unsupported-instruction, and unsupported-terminator diagnostic call sites.
