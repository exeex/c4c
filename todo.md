Status: Active
Source Idea Path: ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Call Lowering And ABI-Binding Helpers

# Current Packet

## Just Finished

Completed Step 3 call lowering and ABI-binding ownership audit. AST-backed
checks show the valid normal-call lowering bodies now live in `calls.cpp`:
prepared call-plan lookup/requirement, before/after call-boundary move
lowering, prepared argument/result ABI-binding lookup, prepared register
conversion for call-boundary moves, direct/indirect call materialization,
memory-return storage from prepared frame facts, clobber/preserved-value effect
construction, and call machine-node construction. No additional Step 3-valid
helper body was moved in this packet because the remaining call-shaped
`dispatch.cpp` body is the retained-call dispatcher wrapper plus variadic entry
helper validation, which is a routing/variadic boundary rather than ABI
authority.

## Suggested Next

Supervisor should choose Step 4 printer/spelling redistribution, keeping direct,
indirect, helper-call, relocation, and call-result spelling movement separate
from generic machine-printer primitives.

## Watchouts

- Step 3 ownership checks:
  - `lower_prepared_call_instruction`, `make_memory_return_storage`,
    `make_indirect_callee_register`, `lower_before_call_moves`,
    `lower_after_call_moves`, `find_prepared_call_plan`, and
    `require_prepared_call_plan` resolve to `calls.cpp`.
  - `lower_before_call_move` / `lower_after_call_move` stay calls-local and use
    calls-local prepared argument/result lookup, ABI binding lookup, F128
    carrier validation, prepared register conversion, and boundary
    machine-instruction construction.
  - `lower_prepared_call_instruction` consumes `PreparedCallPlan` facts for
    direct/indirect callee identity, memory-return storage, prepared arguments,
    prepared result, preserved values, and clobbered registers; it does not
    reclassify ABI locally or recover facts from assembly text.
  - `make_call_instruction` builds call defs/uses/clobbers/preserves from the
    structured call record and prepared clobber/preserved-value carriers.
- Remaining `dispatch.cpp` boundaries after Step 3:
  - `lower_call_instruction` remains dispatch-local because it is the retained
    BIR-call route that sequences variadic entry validation, calls-owned
    `require_prepared_call_plan`, and calls-owned
    `lower_prepared_call_instruction`.
  - `variadic_entry_helper_kind`, `require_prepared_variadic_entry_plan`,
    `variadic_helper_operand_homes_complete`, and related missing-fact message
    helpers remain dispatch-local because moving them would widen Step 3 into
    variadic shard ownership and helper semantics.
  - `append_call_diagnostic` remains duplicated in `dispatch.cpp` for
    dispatch-owned inline asm, intrinsic, and variadic route diagnostics;
    calls-owned normal-call diagnostics use the separate calls-local helper.
  - `dispatch_prepared_block` remains the top-level owner for prepared-block
    traversal, retained-call filtering, and sequencing before-call moves, the
    lowered call, and after-call moves.
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
  - `dispatch.cpp` still owns `lower_call_instruction`, inline asm lowering,
    intrinsic lowering, variadic entry helper validation, and the top-level
    prepared-block routing.

## Proof

Proof passed with log at `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
The final run passed 139/139 `backend_` tests.
