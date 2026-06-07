# Current Packet

Status: Active
Source Idea Path: ideas/open/120_aarch64_calls_after_call_result_value_local_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Local Owner Surface In Place

## Just Finished

Step 1 characterized the current `lower_after_call_move` cluster before any
code movement.

Cluster map:

- Direct caller: `lower_after_call_moves` calls `lower_after_call_move` for
  each `AfterCall` move bundle entry, then may synthesize one after-call result
  publication when `call_plan.result` is register-to-stack and no bundle move
  already publishes that result.
- Direct prepared/shared inputs consumed by `lower_after_call_move`:
  `find_indexed_prepared_value_home` for the destination home,
  `classify_prepared_call_boundary_move` for result-plan and ABI binding
  authority, `find_prepared_f128_carriers` and `find_prepared_f128_carrier` for
  destination f128 carrier facts, the `PreparedCallPlan` result plan, the
  `PreparedMoveBundle`, and each `PreparedMoveResolution`.
- Register-to-register result path: requires `AfterCall`,
  `CallResultAbi`, register destination storage, prepared destination register
  home, matching `PreparedCallResultPlan`, matching ABI binding, and selected
  GPR, scalar FPR, or complete f128 VREG carrier authority. It builds a
  `CallBoundaryMoveInstructionRecord` with source and destination
  `RegisterOperand`s.
- Register-to-frame-slot result path: handles prepared register-source /
  stack-slot-destination result plans. It validates the prepared destination
  stack home, source ABI register, binding consistency, selected source view,
  value type, frame-slot id, offset, size, and alignment, then emits a
  `MemoryInstructionRecord` store to a `FrameSlot` `MemoryOperand`.
- FPR fallback path: if the full result-plan path has not populated both
  registers, an after-call result move with an FPR ABI binding and register
  destination home can still produce source and destination FPR operands from
  the prepared binding and prepared home.
- Post-result ordering in `lower_after_call_moves`: after result publication it
  restores outgoing stack reservation with an `add sp, sp, #...` instruction
  when needed, then appends preservation republication effects planned by
  shared `plan_prepared_call_boundary_effects`.

Direct helper relationships:

- Prepared/shared authority lookups: `prepare::find_indexed_prepared_value_home`,
  `prepare::classify_prepared_call_boundary_move`,
  `prepare::find_prepared_f128_carriers`, and
  `prepare::find_prepared_f128_carrier`.
- AArch64-local register and view helpers:
  `F128CarrierCallOperandOwner::is_complete_full_width`,
  `F128CarrierCallOperandOwner::q_register_operand`,
  `scalar_fp_view_from_register_name`, `scalar_view_from_register_name`,
  `scalar_size_from_register_view`,
  `scalar_integer_register_view_from_size`,
  `scalar_integer_type_from_size`,
  `register_name_with_expected_view`, and
  `make_register_operand_from_prepared_authority`.
- AArch64-local record constructors:
  `make_call_boundary_move_instruction`,
  `make_call_boundary_machine_instruction`,
  `make_memory_instruction`, and `make_register_operand`.

Prepared inputs that must remain inputs, not be recomputed locally:

- Result lane/result plan identity: `PreparedCallResultPlan` and its
  instruction index, source/destination storage kinds, value id, register
  banks, register names, placements, contiguous widths, occupied registers, and
  stack destination facts.
- After-call ABI binding identity: `PreparedAbiBinding` selected by
  `classify_prepared_call_boundary_move`.
- Destination homes: `PreparedValueHome` selected through prepared value-home
  lookups, including register home or stack-slot home, value id/name, slot id,
  offset, size, and alignment.
- f128 carrier facts: complete full-width VREG carrier with q-register name,
  vector class, contiguous width, occupied registers, total size/alignment, and
  no missing required facts.

AArch64-local responsibilities to preserve:

- Parse and view AArch64 register names as GPR, scalar FPR, or q-register
  operands.
- Choose register views for scalar FP, scalar integer widths, and f128 VREG
  carriers from already-prepared facts.
- Spell q/f128 operands through `F128CarrierCallOperandOwner`.
- Spell frame-slot memory operands and call-result store records from prepared
  stack-home facts.
- Construct call-boundary move records, machine instruction wrappers, and
  memory store records with AArch64 target opcode/family/resource metadata.

Focused proof subset candidates:

- `backend_aarch64_instruction_dispatch`: covers GP register result move
  (`block_dispatch_lowers_prepared_register_result_move_after_direct_call`),
  scalar FP/HFA lane result moves
  (`block_dispatch_lowers_prepared_hfa_result_lanes_from_distinct_fprs`), HFA
  lane stores from ABI publication
  (`block_dispatch_stores_hfa_call_result_lanes_from_abi_publication`), and
  ordering of result publication before preservation republication.
- `backend_aarch64_call_boundary_owner`: directly covers GP frame-slot result
  publication, FPR/HFA lane frame-slot publication without an explicit move
  bundle, and f128/q-register frame-slot result publication.
- `backend_prepare_frame_stack_call_contract`: covers prepared after-call ABI
  binding placement and result move placement publication from the shared
  prepared contract.
- `backend_prealloc_call_boundary_classification`: covers shared
  `classify_prepared_call_boundary_move` result-plan and ABI-binding matching.
- `backend_aarch64_target_instruction_records`: useful as a low-level record
  shape guard for call-boundary/memory records.
- Suggested focused CTest regex for the next code packet:
  `^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_prepare_frame_stack_call_contract|backend_prealloc_call_boundary_classification|backend_aarch64_target_instruction_records)$`.

Missing or thin coverage:

- No clearly isolated after-call f128/vector-carrier register-to-register
  result move test was found. Existing f128 coverage proves q-register
  call-boundary argument selection and f128 q-register result publication to a
  frame slot, while register-home after-call result coverage is strongest for
  GPR and scalar FPR/HFA lanes.

## Suggested Next

Execute Step 2: extract an in-place AArch64-local after-call result owner
surface in `src/backend/mir/aarch64/codegen/calls.cpp`. Keep the wrapper as the
prepared-input collection point, pass result plan, ABI binding, destination
home, destination f128 carrier, bundle, move, and diagnostics explicitly, and
preserve behavior before considering any file split.

## Watchouts

- Do not reopen destination-home, stack frame-slot, or f128 carrier authority
  from closed ideas.
- Do not move target-specific ABI register spelling, register-view policy,
  q/f128 rendering, or memory operand spelling into shared prepared code.
- Treat expectation downgrades, unsupported-path rewrites, and named-case-only
  proof as route failures.
- Keep the synthetic register-to-stack result publication path in
  `lower_after_call_moves` intact; it is a real after-call result route, not
  dead compatibility code.
- If Step 2 touches f128 register-result handling, consider adding or requesting
  a focused f128 after-call register-to-register result proof instead of
  claiming that current frame-slot f128 coverage proves that exact shape.

## Proof

No build or test proof was required or run. This was a docs/analysis-only
packet and only `todo.md` changed, so no `test_after.log` was generated.
