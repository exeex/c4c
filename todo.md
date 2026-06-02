Status: Active
Source Idea Path: ideas/open/94_aarch64_calls_f128_carrier_operand_owner.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route F128 Call-Boundary Uses Through The Owner

# Current Packet

## Just Finished

Completed plan Step 3: `Route F128 Call-Boundary Uses Through The Owner`.

Audited the remaining f128 call-boundary references in
`src/backend/mir/aarch64/codegen/calls.cpp` with clang-backed callees plus a
focused text pass. Routed the remaining in-scope after-call f128 result
destination q-register rendering through
`F128CarrierCallOperandOwner::q_register_operand`, and made the owner diagnostic
text neutral because it now covers both source and destination carriers.

The remaining f128 references intentionally stay outside the owner:

- Prepared carrier selection through `find_prepared_f128_carriers` and
  function-local `find_prepared_f128_carrier` stays outside because the owner
  consumes selected carrier facts rather than choosing or creating them. Only
  the module fallback lookup remains owner-mediated.
- The ABI result source operand in `lower_after_call_move` stays outside
  because it is an ABI binding/result-plan operand, not a prepared f128 carrier
  operand.
- F128 stack argument transport stays outside because
  `make_prepared_f128_carrier_transport_record` and
  `make_f128_transport_instruction` are f128 transport authority, not calls-side
  q-register operand rendering.
- F128 constant argument publication keeps its destination register rendering
  and `CallBoundaryMoveInstructionRecord` fields outside because the source is
  a constant payload carrier and the destination is ABI call-argument
  publication/record construction, not a prepared carrier q-register operand.
- Shared scalar FP helpers, ordinary scalar FP moves, immediate scalar
  publication, byval/aggregate transport, and call-boundary record construction
  remain outside this owner.

## Suggested Next

Proceed to Step 5 close-readiness review if the supervisor accepts the focused
proof as satisfying Step 4 for this narrow routing packet. Confirm no shared
scalar FP helpers, record schema, CMake wiring, public declarations, prealloc
files, tests, or unrelated call clusters were swept into this owner.

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
- The owner's q-register diagnostic text changed from source-specific to
  carrier-neutral only because after-call destination carriers now use the same
  operand renderer.

## Proof

Ran the supervisor-selected proof command exactly for the code-changing packet:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_prepared_printer|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result)$' --output-on-failure > test_after.log
```

Result: passed. `test_after.log` records 5/5 tests passed.

Proof log: `test_after.log`.
