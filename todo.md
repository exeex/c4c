Status: Active
Source Idea Path: ideas/open/572_rv64_same_module_call_result_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish Integer/GPR Call Results

# Current Packet

## Just Finished

Completed Step 4 audit for ordinary same-module integer/GPR call-result
publication. No code or test change was needed.

- The exact register-result publication path is
  `fragment_for_prepared_call(...)` after the emitted `R_RISCV_CALL_PLT`
  call pair: require `call.result` and `call_plan->result` to agree; require a
  register-to-register GPR result plan with `destination_value_id`,
  `source_register_name`, and `destination_register_name`; reject floating
  result types; resolve the ABI result source with
  `rv64_register_number(result.source_register_name)`; resolve the owner home
  with `prepared_value_home_for_id(lookups, result.destination_value_id)`; map
  that register home with `gpr_register_number_for_home(...)`; require the
  prepared home to be a register matching the planned destination; then publish
  by `append_rv64_move(fragment, destination, source)`.
- `builds_prepared_immediate_null_same_module_call_object()` proves the
  integer call result is copied from `a0` to the owner GPR and then used by the
  later return move.
- `builds_prepared_prior_result_multi_gpr_same_module_call_object()` proves a
  first call result is published from `a0` to `%first`, then consumed as a GPR
  argument to a second same-module call, and also proves the second call result
  is published before later return use.

## Suggested Next

Run the Step 5 representative evidence packet for `src/20000412-2.c` and
`src/20000622-1.c`.

## Watchouts

- This plan is limited to ordinary same-module RV64 call/result lowering.
- Do not treat `llvm.inline_asm` carriers as ordinary calls.
- Do not implement select, floating-point binary, pointer arithmetic,
  prepared-authority, broad ABI, or runtime-comparison work here.
- Do not add filename-specific matching for `src/20000412-2.c` or
  `src/20000622-1.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Unsupported ordinary call ABI forms still fail closed through the existing
  generic `unsupported_instruction_fragment` surface in this bounded packet;
  precise call-specific diagnostics were not implemented here.
- Register-destination GPR call results are covered by the focused tests. The
  audited path rejects missing result owners/homes, non-register destinations,
  non-GPR banks, mismatched prepared homes, and floating result types by
  returning `std::nullopt` to the existing fail-closed diagnostic surface.
- Stack-slot, FP, varargs, aggregate, and broad external-call result forms are
  outside this Step 4 packet.
- `emit_riscv_simple_call(...)` also allows `DirectExternFixedArity`; the 572
  slice should keep same-module semantics as the proof focus and avoid
  widening into broad external ABI policy.

## Proof

Run:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: build succeeded; focused `backend_riscv_object_emission` CTest passed.

Proof log: `test_after.log`.
