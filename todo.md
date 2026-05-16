Status: Active
Source Idea Path: ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Calls Ownership Surfaces

# Current Packet

## Just Finished

Completed Step 1: inspected the current AArch64 call ownership surfaces and
mapped behavior-preserving extraction targets for future `calls.cpp` /
`calls.hpp` work. The repo currently has `calls.md` only; there is no compiled
`src/backend/mir/aarch64/codegen/calls.cpp` or `calls.hpp` yet.

## Suggested Next

Step 2 first packet: create `src/backend/mir/aarch64/codegen/calls.hpp` and
`calls.cpp`, add `calls.cpp` to `src/backend/CMakeLists.txt`, and move only the
smallest construction/status surface first:
`call_selection_status`, `make_call_boundary_move_instruction`,
`make_call_boundary_abi_binding_instruction`, `make_call_instruction`, and the
call clobber/preserved-value effect helpers needed by `make_call_instruction`.

## Watchouts

- Concrete extraction targets found by AST-backed inventory:
  - `instruction.cpp`: move call-family construction/status bodies from the
    broad instruction owner, including `call_selection_status`,
    `make_call_boundary_move_instruction`,
    `make_call_boundary_abi_binding_instruction`, `make_call_instruction`,
    `effect_from_prepared_call_clobber`,
    `effects_from_prepared_call_clobbers`,
    `effect_from_prepared_call_preserved_value`, and
    `effects_from_prepared_call_preserved_values`.
  - `dispatch.cpp`: move call-family lowering and prepared-fact lookup bodies
    into the calls owner, including `append_call_diagnostic`,
    `find_prepared_call_plan`, `find_prepared_argument_plan`,
    `find_prepared_argument_binding`, `find_prepared_result_binding`,
    `make_register_operand_from_prepared_authority`,
    `complete_full_width_f128_carrier`, `complete_f128_constant_carrier`,
    `lower_before_call_move`, `lower_after_call_move`,
    `lower_before_call_moves`, `lower_after_call_moves`,
    `make_indirect_callee_register`, `make_memory_return_storage`, and
    `lower_call_instruction`.
  - `machine_printer.cpp`: move call spelling bodies to calls ownership,
    including `print_call`, `print_call_boundary_move`,
    `print_call_boundary_abi_binding`, and the direct `bl` / indirect `blr`
    structured spelling that `print_call` owns. Keep helper-call spelling as a
    later boundary decision because `print_i128_runtime_helper`,
    `print_f128_runtime_helper`, `append_i128_helper_move_line`, and
    `append_f128_helper_move_line` currently belong to runtime-helper families
    but emit terminal helper calls.
  - `src/backend/CMakeLists.txt`: when Step 2 creates `calls.cpp`, add it next
    to nearby AArch64 codegen shard sources.
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
  - Existing `alu.*`, `returns.*`, `comparison.*`, and `operands.*` shard
    patterns are the nearest local convention for `calls.hpp` / `calls.cpp`.
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

Read-only inspection packet; no build or tests were run and `test_after.log`
was not modified. Supervisor-selected future code proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
