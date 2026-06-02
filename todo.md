Status: Active
Source Idea Path: ideas/open/94_aarch64_calls_f128_carrier_operand_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce The F128 Carrier Operand Owner

# Current Packet

## Just Finished

Completed plan Step 2: `Introduce The F128 Carrier Operand Owner`.

Implemented a local `F128CarrierCallOperandOwner` in
`src/backend/mir/aarch64/codegen/calls.cpp` and grouped only the f128-specific
calls-side carrier operand helpers behind it:

- `is_complete_full_width` now owns the previous full-width f128 carrier
  completion predicate.
- `is_complete_constant` now owns the previous f128 constant carrier completion
  predicate.
- `find_in_module` now owns the module fallback lookup for a prepared f128
  carrier by `PreparedValueId`.
- `q_register_operand` now owns prepared f128 carrier to AArch64 q-register
  operand rendering and preserves the existing diagnostics.

Updated the in-scope call sites in `lower_before_call_move` and
`lower_after_call_move` to consume the owner. Shared scalar FP helpers,
generic register operand rendering, prepared carrier selection, and
call-boundary record construction remain outside the owner.

## Suggested Next

Execute Step 3 by checking whether any remaining f128 call-boundary uses should
be routed through `F128CarrierCallOperandOwner`, or documenting why they stay
outside. Keep the next packet limited to calls-side f128 carrier completion,
module lookup consumption, and q-register operand rendering.

## Watchouts

- Keep this route limited to AArch64 calls-side f128 carrier completion and
  q-register operand rendering.
- Leave `scalar_fp_view_from_register_name`,
  `scalar_view_from_register_name`, `scalar_size_from_register_view`,
  `register_name_with_expected_view`, and
  `make_register_operand_from_prepared_authority` outside the owner because
  they are shared by non-f128 scalar GP/FPR call-boundary paths.
- Do not move prepared f128 carrier creation, prepared carrier selection,
  shared transport authority, aggregate byval lane transport, immediate scalar
  publication, ordinary scalar FP moves, or call-boundary record ownership into
  the new owner.
- `lower_before_call_move` should keep deciding whether a move is an argument
  move, constant argument move, ordinary scalar FPR move, frame-slot move, or
  record publication. `lower_after_call_move` should keep result classification
  and record publication.
- Preserve diagnostics, q-register selection, prepared carrier contracts,
  record fields, and assembly output.
- Keep idea `95` separate unless a supervisor-approved lifecycle switch
  happens.

## Proof

Ran the supervisor-selected proof command exactly:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_prepared_printer|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result)$' --output-on-failure > test_after.log
```

Result: passed. `test_after.log` records 5/5 tests passed.

Proof log: `test_after.log`.
