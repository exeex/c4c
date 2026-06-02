Status: Active
Source Idea Path: ideas/open/94_aarch64_calls_f128_carrier_operand_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm F128 Carrier Boundary And Proof Subset

# Current Packet

## Just Finished

Completed plan Step 1: `Confirm F128 Carrier Boundary And Proof Subset`.

Boundary audit:

- `complete_full_width_f128_carrier` consumes prepared f128 carrier facts and
  currently gates full-width structured q-register argument/result moves from
  `lower_before_call_move` and `lower_after_call_move`.
- `complete_f128_constant_carrier` consumes prepared f128 carrier facts plus
  exact low/high constant payload facts and currently gates immediate f128
  argument publication from `lower_before_call_move`.
- `find_prepared_f128_carrier_in_module` is a calls-side fallback lookup for a
  prepared f128 carrier by `PreparedValueId` when the current function carrier
  table did not resolve the direct-call source.
- `make_f128_q_register_operand_from_carrier` renders a prepared full-width
  f128 carrier as an AArch64 q-register `RegisterOperand`, preserving prepared
  bank/class/width/occupancy and diagnostics for non-FP/SIMD or non-q-viewable
  carrier registers.
- `scalar_fp_view_from_register_name` is shared by ordinary scalar FPR
  argument/result moves and scalar frame-slot argument loads; it should not be
  moved into the f128 owner except through a narrow f128-only wrapper if q-view
  conversion needs one.

Smallest proposed owner boundary:

- Add a local `F128CarrierCallOperandOwner` in `calls.cpp`.
- Move/consolidate only these f128-specific helpers behind it:
  `complete_full_width_f128_carrier`,
  `complete_f128_constant_carrier`,
  `find_prepared_f128_carrier_in_module`, and
  `make_f128_q_register_operand_from_carrier`.
- Expose narrow methods such as `is_complete_full_width`,
  `is_complete_constant`, `find_in_module`, and `q_register_operand`.
- Inputs should be prepared carrier/function/module facts, payload/register
  facts, operand role/value ids/names, and diagnostics context. Outputs should
  remain booleans, a prepared carrier pointer, or a rendered q-register operand.
- Keep prepared carrier creation/selection, call argument/result classification,
  move-record construction, generic register operand rendering, and assembly
  emission outside the owner.

## Suggested Next

Execute Step 2 by introducing the local `F128CarrierCallOperandOwner` in
`src/backend/mir/aarch64/codegen/calls.cpp` and routing only the audited
helper bodies through it. Do not change behavior or public declarations unless
an existing external call path requires it.

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

Audit-only packet; no build/tests required because no implementation files were
touched.

Local validation for this packet:

`git diff --name-only`

Expected result: only `todo.md` changed.

Focused proof command to use after implementation:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_prepared_printer|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result)$' --output-on-failure > test_after.log
```

Subset coverage:

- `backend_aarch64_call_boundary_owner`: full-width f128 HFA/call-boundary
  q-register authority.
- `backend_aarch64_target_instruction_records`: f128 call-boundary move record
  q-register selection, f128 transport records, missing/incomplete f128 carrier
  diagnostics, and adjacent scalar FPR call-boundary move records.
- `backend_aarch64_instruction_dispatch`: prepared f128 argument q-register
  moves, frame-slot f128 argument routes, f128 constant argument carrier
  consumption, missing/incomplete carrier rejection paths, and representative
  q-register assembly.
- `backend_prepared_printer`: prepared f128 carrier dump contracts for memory,
  full-width register, and constant payload facts.
- `backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result`:
  adjacent ordinary scalar FP route to catch accidental ownership bleed into
  non-f128 FPR publication.
