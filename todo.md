Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Split Or Classify Compatibility Projection

# Current Packet

## Just Finished

Plan repair updated Step 4 and Step 5 away from retired module draft filenames
and toward the current `src/backend/mir/aarch64/codegen/*` implementation
surfaces.

`plan.md` now treats the completed extraction/migration work as current-state
context, keeps `machine_printer.*` deferred to idea 224 shared MIR printer
cleanup, and narrows active Step 6 to compatibility projection
split/classification.

## Suggested Next

Next executor packet: Step 6, split the compatibility projection currently in
`src/backend/mir/aarch64/codegen/emit.cpp` into target-private
`src/backend/mir/aarch64/codegen/compatibility_projection.hpp` and
`src/backend/mir/aarch64/codegen/compatibility_projection.cpp`, with build
wiring and focused proof. If inspection shows the split is not warranted, leave
the projection in `emit.cpp` but classify it clearly as derived compatibility,
not semantic lowering authority, and record the reason here.

## Watchouts

- Do not recreate deleted `module/function_traversal.cpp`,
  `module/operand_resolution.cpp`, `module/instruction_lowering.cpp`,
  `module/branch_control_lowering.cpp`, or `module/call_lowering.cpp`.
- `machine_printer.*` is still live code today; deleting it before the shared
  MIR printer route lands would break the public AArch64 asm path.
- Idea 224 owns replacing this target-local printer with common MIR traversal
  plus AArch64 target rendering hooks.
- The compatibility projection must remain derived from canonical lowering; it
  must not become a fallback lowering input or a broad record-pile owner.
- No implementation, lowering expectations, or testcase contracts were changed
  in this lifecycle repair packet.

## Proof

Lifecycle-only repair. Reference hygiene to run after this patch:
`rg -n 'src/backend/mir/aarch64/module/(function_traversal|operand_resolution|instruction_lowering|branch_control_lowering|call_lowering|public_assembly_bridge|compatibility_projection)' plan.md todo.md`.
