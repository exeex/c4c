Status: Active
Source Idea Path: ideas/open/265_aarch64_asm_emitter_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Or Tighten Current-Route Ownership

# Current Packet

## Just Finished

Step 2 - Move Or Tighten Current-Route Ownership completed.

No code move was needed. Focused review confirmed the current AArch64 public
`.s` route is already owned across the intended boundary:
`asm_emitter.cpp`/`.hpp` own the prepared-module-to-assembly translation-unit
wrapper, including the `print_prepared_machine_nodes(...)` surface, prepared
module compilation, `.text`, `.globl`, `.type`, function labels, `.size`, the
selected-node/empty-route guards, and `.note.GNU-stack` emission.
`machine_printer.*` owns target instruction payload spelling through
`MachineInstructionPrinter` and
`print_machine_instruction_line_payloads(...)`, which is the correct sibling
owner rather than a stranded wrapper helper.

`backend.cpp` only holds the public BIR/LIR handoff calls into
`print_prepared_machine_nodes(...)`; no current-route assembly rendering helper
was found stranded there or elsewhere in the focused AArch64 codegen/public
handoff surface. No behavior changes were made.

## Suggested Next

Start Step 3 by deleting
`src/backend/mir/aarch64/codegen/asm_emitter.md`, then confirm no source list,
docs index, or build/test reference still expects that stale shard. Keep
neighboring markdown shards untouched.

## Watchouts

- Preserve emitted `.s` behavior.
- Do not port legacy inline-assembly reference machinery.
- Do not redistribute neighboring AArch64 markdown shards.
- Do not weaken tests or smoke coverage.
- Keep `machine_printer.*` instruction-spelling logic separate from
  `asm_emitter.cpp`; Step 2 confirmed that boundary is intentional.

## Proof

Ran:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed. The build completed and the backend subset reported 139/139
tests passing. Proof log: `test_after.log`.
