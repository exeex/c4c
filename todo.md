Status: Active
Source Idea Path: ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Calls Owner And Move Construction Routes

# Current Packet

## Just Finished

Completed Step 2 second packet: moved the smallest call lowering route from
`dispatch.cpp` into the compiled calls owner. `calls.cpp` / `calls.hpp` now own
prepared call-plan lookup plus before/after call-boundary move lowering,
including the prepared argument/result binding lookup, structured F128 carrier
checks, prepared register conversion, call-boundary diagnostics, and
call-boundary `MachineInstruction` construction. `dispatch.cpp` now routes to
the calls-owned helpers and keeps block traversal, inline asm/intrinsic routing,
the main call body, indirect callee lowering, memory-return storage, and
non-call families.

## Suggested Next

Step 2 next packet: move one more narrow call route from `dispatch.cpp` into
`calls.cpp`, preferably the call body pieces that directly consume the
calls-owned prepared plan (`make_memory_return_storage`,
`make_indirect_callee_register`, and/or direct-callee record assembly) while
leaving variadic helper validation, inline asm, and intrinsic dispatch outside
the packet unless explicitly owned.

## Watchouts

- AST-backed ownership checks after this packet:
  - `lower_before_call_moves` resolves to `calls.cpp`.
  - `find_prepared_call_plan` resolves to `calls.cpp`.
  - `dispatch_prepared_block` is the direct caller of calls-owned
    `lower_before_call_moves`; `lower_after_call_moves` follows the same route.
  - `lower_before_call_move` callees now resolve to calls-owned local helpers
    for prepared argument lookup, ABI binding lookup, F128 carrier validation,
    call-boundary diagnostics, prepared register conversion, and boundary
    machine-instruction construction.
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
  - `dispatch.cpp` still owns `lower_call_instruction`,
    `make_memory_return_storage`, `make_indirect_callee_register`, inline asm
    lowering, intrinsic lowering, variadic entry helper validation, and the
    top-level prepared-block routing.

## Proof

Proof passed with log at `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
The final run passed 139/139 `backend_` tests.
