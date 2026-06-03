Status: Active
Source Idea Path: ideas/open/102_aapcs64_va_arg_payload_shape_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Proof

# Current Packet

## Just Finished

Completed `plan.md` Step 4: reviewed and proved focused coverage for scalar,
aggregate, and HFA AAPCS64 `va_arg` payload authority.

### Step 4 Coverage

- Scalar GP `va_arg` proof:
  - `backend_prepared_printer` asserts the hand-built `llvm.va_arg.i32`
    placeholder carries explicit payload metadata and produces a prepared
    scalar access plan with `gp_register_save_area`, `type=i32`, `size=4`,
    and `align=4`.
  - `backend_prepare_frame_stack_call_contract` independently asserts the
    `i32` access-plan carrier facts survive frame/stack planning.
  - `backend_aarch64_target_instruction_records` asserts prepared scalar
    access plans are required and selected by AArch64 call instruction records.
- Scalar FP `va_arg` proof:
  - `backend_prepared_printer` asserts the hand-built `llvm.va_arg.f64`
    placeholder carries explicit payload metadata and produces a prepared
    scalar access plan with `fp_register_save_area`, `type=f64`, `size=8`,
    and `align=8`.
  - `backend_prepare_frame_stack_call_contract` independently asserts the
    `f64` access-plan carrier facts survive frame/stack planning.
  - `backend_aarch64_target_instruction_records` asserts FP scalar access-plan
    selection for AArch64.
- Non-HFA aggregate proof:
  - `backend_prepare_liveness` asserts lowered aggregate `va_arg` BIR publishes
    explicit `va_arg_payload_abi` size/alignment and sret-style payload
    metadata.
  - `backend_prepared_printer` and
    `backend_prepare_frame_stack_call_contract` assert prepared aggregate plans
    preserve payload size/align, destination-home behavior, overflow source
    field, copy size/align, and progression stride.
  - `backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy`
    proves the backend route still handles the aggregate overflow byte-copy
    case.
- HFA aggregate proof:
  - `backend_lir_to_bir_notes` asserts BIR runtime lowering publishes
    `va_arg_payload_abi`, `va_arg_hfa_lane_count`, and
    `va_arg_hfa_lane_size_bytes` for the HFA aggregate helper.
  - `backend_prepared_printer` asserts prealloc consumes explicit lane
    count/size by producing a register-save-area aggregate access plan and
    dumping `register_save_lanes=1` plus `register_save_lane_size=4`.

No additional test edits were needed in this packet because the Step 3
assertions already covered the required scalar GP/FP, non-HFA aggregate, and
HFA lane-shape proof surfaces.

## Suggested Next

Proceed to Step 5: run final validation and prepare close-readiness notes.

## Watchouts

- Do not add or endorse slot-name parsing as payload-shape authority in any
  follow-up proof or final-validation work.
- Keep fixed-call HFA pressure and unrelated AArch64 ABI behavior out of scope.
- Runtime helper identity by raw `llvm.va_arg.*` spelling remains a separate
  placeholder-identity concern; this plan covers payload ABI/shape authority.

## Proof

Command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_lir_to_bir_notes|backend_prepare_liveness|backend_prepared_printer|backend_aarch64_target_instruction_records|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy)$') > test_after.log 2>&1
```

Result: passed. `test_after.log` contains the delegated proof output.

Summary:

- Build: `ninja: no work to do.` during the captured proof command.
- CTest subset: `6/6` passed.
- Passing tests: `backend_prepare_frame_stack_call_contract`,
  `backend_lir_to_bir_notes`, `backend_prepare_liveness`,
  `backend_prepared_printer`, `backend_aarch64_target_instruction_records`, and
  `backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy`.
