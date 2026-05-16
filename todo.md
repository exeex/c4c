Status: Active
Source Idea Path: ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Calls Owner And Move Construction Routes

# Current Packet

## Just Finished

Completed Step 2 first packet: created compiled
`src/backend/mir/aarch64/codegen/calls.cpp` / `calls.hpp`, added `calls.cpp` to
`src/backend/CMakeLists.txt`, and moved the call construction/status surface
out of `instruction.cpp`: `call_selection_status`,
`make_call_boundary_move_instruction`,
`make_call_boundary_abi_binding_instruction`, `make_call_instruction`,
`effect_from_prepared_call_clobber`,
`effects_from_prepared_call_clobbers`,
`effect_from_prepared_call_preserved_value`, and
`effects_from_prepared_call_preserved_values`.

## Suggested Next

Step 2 next packet: move the smallest call lowering route from `dispatch.cpp`
into the compiled calls owner, starting with prepared call-plan lookup and
boundary move lowering only if the supervisor explicitly owns `dispatch.cpp`
for that packet.

## Watchouts

- AST-backed ownership checks after this packet:
  - `make_call_instruction` resolves to `calls.cpp`; the same definition query
    against `instruction.cpp` reports the symbol absent from that translation
    unit.
  - `make_call_instruction` callees now include calls-owned
    `call_selection_status`,
    `effects_from_prepared_call_clobbers`, and
    `effects_from_prepared_call_preserved_values` in `calls.cpp`.
  - `instruction.cpp` still calls the public calls-owned
    `effects_from_prepared_call_clobbers` for existing F128/i128 runtime helper
    boundary records; do not move helper-call ownership unless a later packet
    explicitly owns that boundary.
- Family-neutral boundaries to keep outside calls ownership:
  - `instruction.hpp` must keep shared enums, variant payload declarations,
    operand records, `InstructionRecord`, and public factory declarations until
    a broader header split is explicitly planned.
  - `instruction.cpp` should retain generic name tables, opcode/mnemonic
    mapping, operand factories, and non-call family constructors.
  - `dispatch.cpp` should retain block traversal, instruction-family
    classification, non-call diagnostics, scalar state threading, and routing;
    after extraction it should call into calls-owned lowering instead of owning
    the call body.
  - `machine_printer.cpp` should retain generic printer entry points, register
    and memory spelling primitives, relocation helpers, and the variant routing
    switch; after extraction it should route call payloads to calls-owned
    printer helpers.
- Carrier gaps and blockers to preserve, not patch locally:
  - `calls.md` says outgoing stack snapshots, indirect-callee spill/materialize
    offsets, call-time alignment, F128/i128/aggregate/variadic classification,
    memory-return `x8`, clobbers, preserved values, and helper-call resources
    must come from prepared call plans, move bundles, ABI-binding records,
    allocation records, and the AAPCS64/ALLOCATION contracts.
  - The current compiled path already has structured carriers for direct and
    indirect callee identity, before/after call moves, prepared argument/result
    plans, memory-return storage, preserved values, clobbers, variadic helper
    operand homes, and selected helper-call ownership. Missing total outgoing
    stack area, indirect spill-area, or call-time alignment facts remain carrier
    gaps; do not infer them from legacy `calls.md` text.
  - Do not delete `calls.md` until Step 5 proves every durable entry point,
    hidden assumption, failure risk, and rebuild-guidance item is either in the
    compiled calls owner or recorded as an active carrier gap.
  - Do not move variadic entry helper semantics wholesale into calls unless the
    packet explicitly owns that boundary; they are call-adjacent but also part
    of the variadic shard.

## Proof

Proof passed with log at `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
The final run built `calls.cpp` and passed 139/139 `backend_` tests.
