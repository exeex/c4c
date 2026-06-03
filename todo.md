Status: Active
Source Idea Path: ideas/open/102_aapcs64_va_arg_payload_shape_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Contract

# Current Packet

## Just Finished

Completed `plan.md` Step 3: implemented the AAPCS64 `va_arg`
payload-shape authority contract.

### Step 3 Implementation

- Added named runtime `va_arg` payload metadata to `bir::CallInst`:
  `va_arg_payload_abi`, `va_arg_hfa_lane_count`, and
  `va_arg_hfa_lane_size_bytes`.
- Populated scalar placeholder payload ABI in
  `src/backend/bir/lir_to_bir/calling.cpp::lower_runtime_intrinsic_inst` from
  `compute_call_arg_abi(...)` at BIR runtime lowering time.
- Populated aggregate placeholder payload ABI from the existing aggregate
  layout used to build `llvm.va_arg.aggregate`.
- Added BIR-side HFA payload-shape detection from aggregate layout, publishing
  lane count and lane size on the runtime placeholder instead of leaving those
  facts for prealloc to infer from post-call load shape.
- Updated `src/backend/prealloc/variadic_entry_plans.cpp` so scalar access
  planning consumes `call.va_arg_payload_abi` instead of silently recomputing
  `infer_call_arg_abi(target_profile, call.return_type)`.
- Updated aggregate/HFA planning so payload size/align and HFA lane count/size
  come from the named BIR placeholder metadata.
- Kept prealloc helper-local reconstruction only for placement facts:
  destination homes, va_list fields, register-save/overflow fields, strides,
  and frame-slot coordinates.
- Reworked the old HFA helper into
  `find_aapcs64_hfa_va_arg_lane_destination_homes(...)`; it now receives the
  explicit expected lane count and lane size and no longer derives those shape
  facts from load/slot/frame patterns.
- Strengthened focused tests:
  - BIR lowering HFA fixture now asserts `va_arg_payload_abi` and HFA lane
    metadata on `llvm.va_arg.aggregate`.
  - Prepare liveness aggregate fixture now asserts explicit payload ABI
    metadata survives lowering.
  - Prepared-printer variadic helper-family fixture now hand-builds scalar and
    aggregate placeholders with explicit payload ABI metadata.
  - Prepared-printer helper-family fixture now includes an HFA aggregate
    `va_arg` path that proves prealloc consumes explicit lane count/size by
    dumping `register_save_lanes` and `register_save_lane_size`.
- Fixed the broader backend guard regression in
  `backend_prepare_frame_stack_call_contract`: its hand-built
  `llvm.va_arg.i32`, `llvm.va_arg.f64`, and `llvm.va_arg.aggregate` fixture
  calls now publish explicit `va_arg_payload_abi` metadata instead of relying
  on prealloc to reconstruct payload ABI silently.

## Suggested Next

Proceed to Step 4: add or strengthen focused proof if the supervisor wants a
separate proof-only packet; otherwise Step 3 already covered the delegated
focused subset for scalar, aggregate, and HFA contract surfaces.

## Watchouts

- Do not add or endorse slot-name parsing as payload-shape authority.
- Keep fixed-call HFA pressure and unrelated AArch64 ABI behavior out of scope.
- Runtime helper identity by raw `llvm.va_arg.*` spelling remains a separate
  placeholder-identity concern; this packet only changed payload ABI/shape
  authority.
- The delegated owned prepared-printer path named
  `tests/backend/prealloc/backend_prepared_printer_test.cpp`, but the actual
  repo test target/file is `tests/backend/bir/backend_prepared_printer_test.cpp`.
- The HFA destination-home mapping still uses existing aggregate slot suffixes
  to find homes, but lane count and lane size are now explicit BIR metadata and
  the helper fails closed when expected homes do not match that metadata.

## Proof

Command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_lir_to_bir_notes|backend_prepare_liveness|backend_prepared_printer|backend_aarch64_target_instruction_records)$') > test_after.log 2>&1
```

Result: passed. `test_after.log` contains the delegated proof output.

Summary:

- Build: rebuilt `backend_prepare_frame_stack_call_contract_test` during the
  captured proof command.
- CTest subset: `5/5` passed.
- Passing tests: `backend_prepare_frame_stack_call_contract`,
  `backend_lir_to_bir_notes`, `backend_prepare_liveness`,
  `backend_prepared_printer`, and `backend_aarch64_target_instruction_records`.
