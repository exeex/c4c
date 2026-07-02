# Current Packet

Status: Active
Source Idea Path: ideas/open/535_rv64_object_frame_stack_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Frame And Stack Helper Surfaces

## Just Finished

Completed Step 1 mapping for the RV64 object-route frame and stack helper
surfaces.

Safe Step 2 extraction boundary:

- Destination owner: `prepared_frame_emit.cpp`/`prepared_frame_emit.hpp`.
- Pure frame sizing helpers to move: `rv64_supported_fixed_frame_alignment`,
  `align_rv64_object_stack_frame_size`, `rv64_find_function_frame_slot`,
  `rv64_frame_slot_extent_is_supported`,
  `rv64_validated_prepared_fixed_frame_size`, and
  `rv64_object_stack_frame_size`.
- Pure stack-slot offset helpers to move: `prepared_stack_slot_home_absolute_offset`
  and `prepared_stack_slot_home_offset`.
- Register-home lookup helpers safe to expose from the frame boundary:
  `rv64_register_number` and `gpr_register_number_for_home`.
- Basic encoded GPR stack helpers safe to move only with object-fragment
  compatibility wrappers preserved in `object_emission.cpp`:
  `rv64_load_store_funct3_for_size`, `append_rv64_store_register_to_stack`,
  `append_rv64_load_stack_to_register`,
  `append_rv64_store_register_to_stack_offset`, and
  `append_rv64_load_stack_offset_to_register`.
- Simple stack adjustment helpers safe to move only with wrappers preserved:
  `append_rv64_stack_pointer_adjustment`, plus its direct primitive dependencies
  `append_rv64_move`, `append_rv64_add_registers`, and
  `append_rv64_load_immediate` if Step 2 chooses to move the adjustment helper.

Compatibility wrappers to preserve in `object_emission.cpp`:

- Keep existing object-route helper names callable from current sites, especially
  `rv64_object_stack_frame_size`, `prepared_stack_slot_home_absolute_offset`,
  `prepared_stack_slot_home_offset`, `rv64_register_number`,
  `gpr_register_number_for_home`, `append_rv64_store_register_to_stack`,
  `append_rv64_load_stack_to_register`,
  `append_rv64_store_register_to_stack_offset`,
  `append_rv64_load_stack_offset_to_register`, and
  `append_rv64_stack_pointer_adjustment`.
- Wrappers should forward to the prepared-frame owner and avoid broad call-site
  churn across calls, moves, local memory, scalar/select, formal entry, and edge
  publication.

Current consumers and prerequisites:

- `rv64_object_stack_frame_size` is consumed by
  `rv64_variadic_function_admission_diagnostic` and
  `prepared_function_to_object_function`; it depends on `PreparedAddressingFunction`,
  `PreparedFramePlanFunction`, `PreparedStackLayout`, saved-register placement
  contract helpers, `<unordered_set>`, `<algorithm>`, `<limits>`, and
  `fits_signed_12_bit_immediate`.
- `prepared_stack_slot_home_absolute_offset` is consumed by move bundles,
  before-return stack-to-register ABI moves, call result publication, casts,
  select/scalar helpers, and formal-entry homes; it requires complete
  `PreparedStackLayout`, `PreparedValueHome`, and frame-slot definitions from
  `../../../prealloc/module.hpp`.
- `prepared_stack_slot_home_offset` is consumed by variadic diagnostics,
  `va_start`, scalar compare trunc publication checks, and its value wrapper.
- GPR stack load/store helpers are consumed by prologue/epilogue, variadic GPR
  publication, calls, local memory, select edge publication, moves, scalar
  helpers, and formal-entry homes; they require `RiscvEncodedFragment` from
  `object_emission.hpp` and RV64 encoder/append primitives. If `prepared_frame_emit.hpp`
  declares them, it must include or forward through the object fragment type
  without pulling in object-module assembly.
- `append_rv64_stack_pointer_adjustment` is consumed by call/stack frame
  prologue and epilogue helpers; moving it requires preserving the scratch
  register value `t0`/x5 and using the existing RV64 encoders exactly.

Unsafe or parked helpers for this idea slice:

- Call-specific helpers stay in `object_emission.cpp`:
  `prepared_frame_slot_call_argument_offset`,
  `prepared_frame_slot_address_call_argument_offset`,
  `prepared_frame_slot_address_call_argument_publication`,
  `stack_slot_offset_for_prior_preserved_gpr_selection`,
  `gpr_register_number_for_prior_preserved_selection`,
  `append_rv64_callee_saved_gpr_preservation_effect`, and
  `rv64_noop_stack_slot_preservation_effect`.
- Formal-entry fragment emission stays in `object_emission.cpp`:
  `make_rv64_formal_entry_home_fragment` can keep calling moved offset/store
  wrappers, but should not move under this Step 2 boundary.
- Local/global memory and pointer materialization helpers stay parked:
  `prepared_stack_slot_home_absolute_offset_for_value`,
  `prepared_stack_slot_home_offset_for_value`,
  `append_rv64_store_register_to_base`,
  `append_rv64_load_base_to_register`,
  `append_rv64_store_register_to_global_base`,
  `append_rv64_load_global_base_to_register`,
  `append_rv64_stack_offset_address_to_register`, local/global load/store
  fragments, symbol materialization, and data-object assembly.
- FPR stack helpers stay parked for now:
  `fpr_register_number_for_home`, FPR move helpers,
  `append_rv64_store_fpr_to_stack`, `append_rv64_load_stack_to_fpr`,
  `append_rv64_store_fpr_to_stack_offset`, and
  `append_rv64_load_stack_offset_to_fpr`.
- Traversal and dispatch stay central: `prepared_function_to_object_function`,
  `fragment_for_prepared_instruction`, terminator dispatch, before-return
  bundles, byval/sret publication, symbol/fixup module assembly, relocation
  handling, and ELF writing.

## Suggested Next

Execute Step 2 by moving the safe helper set above into
`prepared_frame_emit.*` while preserving `object_emission.cpp` compatibility
wrappers and object-route behavior.

## Watchouts

- Keep this behavior-preserving: no frame-size, alignment, ABI, diagnostic,
  unsupported-contract, object-byte, expectation, or RV64 capability changes.
- Do not move call-specific byval/sret publication, before-return bundles,
  local memory semantics, broad function traversal, prepared instruction
  dispatch, data-object assembly, relocation handling, or ELF writing.
- Do not infer missing prepared facts from BIR, target text, object bytes, or
  testcase shape.

## Proof

No build required for this mapping-only packet. Step 2 validation command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_stack_passed_parameter_home_publication|obj_runtime_rv64_large_fixed_frame_slot_access|obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload)'`
