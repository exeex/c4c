Status: Active
Source Idea Path: ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Admit Pointer-Value Scalar Accesses

# Current Packet

## Just Finished

Plan Step 2 (`Admit Pointer-Value Scalar Accesses`) is implemented for the
first supportable RV64 prepared pointer-value local-memory shape, including the
supervisor-found store temporary collision case.

Completed in this packet:

- Added RV64 object emission support for prepared local load/store accesses
  whose address base is `PointerValue`, whose pointer source has a GPR home,
  whose byte offset is RV64 12-bit encodable, and whose metadata is scalar,
  default-address-space, nonvolatile, base-plus-offset, and not over-aligned.
- Fixed pointer-value stores so the temporary used for the stored value does
  not clobber the pointer base register when the base pointer already lives in
  `t1`/`x6`.
- Preserved the existing direct frame-slot local-memory path and extended the
  local-memory diagnostic to describe the two supported address families.
- Added focused object-emission coverage for an i16 pointer-value store/load
  through `%p + 2`, an explicit `t1`/`x6` base collision fixture, plus
  fail-closed neighbors for missing pointer metadata, missing pointer register,
  out-of-range offset, bad alignment, non-default address space, volatile
  access, dynamic/non-base-plus-offset access, mismatched/aggregate-sized
  metadata, and unsupported scalar type.

## Suggested Next

Run Plan Step 3 against the three representatives:

- `src/20000217-1.c`
- `src/20000121-1.c`
- `src/va-arg-13.c`

Record whether each representative now passes, still fails on in-scope
pointer-value local memory, or advances to the frame-slot address
call-argument owner in idea 372.

## Watchouts

- Do not expand idea 368 into call-argument address materialization. If a
  representative advances to `missing_frame_slot_arg_publication` or a similar
  call argument boundary, hand it to idea 372.
- Do not implement aggregate `va_arg`, byval homes, terminator lowering, or
  source-shape shortcuts from this plan.
- The shared local-memory diagnostic now names both admitted address families;
  inspect prepared facts before assigning any remaining failure to this plan.
- Put analysis logs under `build/agent_state/`, not root-level canonical logs.
- Keep `test_after.log` reserved for the supervisor-delegated proof command.

## Proof

Delegated proof passed and wrote `test_after.log`:

`cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log`

Fresh result after the store temporary collision fix: 3/3 tests passed:

- `backend_riscv_object_emission`
- `backend_prepare_frame_stack_call_contract`
- `backend_prepared_printer`
