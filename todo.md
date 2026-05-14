Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time marked the current
`src/backend/mir/aarch64/codegen/machine_printer.*` path as a temporary legacy
terminal assembly printer that should be removed by idea 224's shared MIR
printer boundary work, not grown by the active 228 route.

`plan.md` no longer lists `machine_printer.cpp` as an active Stage 4 read/target
surface. The files remain in the build because `backend.cpp` and focused printer
tests still use them for the current public AArch64 asm route.

## Suggested Next

Supervisor can review and commit this lifecycle clarification, then proceed to
the compatibility projection split or the next route-review packet.

## Watchouts

- `machine_printer.*` is still live code today; deleting it before the shared
  MIR printer route lands would break the public AArch64 asm path.
- Idea 224 owns replacing this target-local printer with common MIR traversal
  plus AArch64 target rendering hooks.
- Review checkpoint: the codegen headers introduced during the extraction
  remain target-private helper surfaces used by focused tests, not stable
  module public API.
- No implementation, lowering expectations, or testcase contracts were changed
  in this packet.

## Proof

Lifecycle-only clarification. Validation is reference hygiene:
`rg -n 'codegen/machine_printer\\.cpp' plan.md todo.md`.
