Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed the Step 5 F128/long-double call-boundary lane-copy slice.

- Verified the real `00204.c` prepared plans for `myprintf` blocks 43, 46, 49,
  and 52 already carry the intended F128 HFA register-save facts:
  `source_class=register_save_area`, `register_save_lanes=1..4`, and
  `register_save_lane_size=16`.
- Found the remaining long-double repetition was downstream of correct
  `llvm.va_arg.aggregate` extraction: each F128 lane was copied from the
  register-save area to a distinct local stack slot, but the following
  variadic `printf` call setup reused the first q-register source for multiple
  ABI destinations.
- Added AArch64 F128 q-register call-boundary materialization in
  `src/backend/mir/aarch64/codegen/calls.cpp`, emitting a real
  `mov vD.16b, vS.16b` when a prepared full-width F128 carrier moves between
  distinct FP/SIMD registers.
- Kept global-backed F128 call arguments on the direct load-to-ABI path by
  tracing same-block F128 values back to global loads before using the
  register-copy shortcut; this preserves existing global HFA route contracts.
- Updated the focused AArch64 call-boundary owner contract to assert the
  emitted `mov v0.16b, v13.16b` byte-lane copy rather than the old
  pre-materialized move payload.
- No broad c-testsuite expectations or `00204.c` dump expectations were
  changed.

## Suggested Next

Delegate the next narrow AArch64 variadic HFA packet to the remaining
float/double overflow-source corruption. The F128/long-double register-save
lane repetition is repaired; the remaining `00204.c` mismatch starts when HFA
float/double arguments spill past the eight FP argument registers and are copied
from caller-side frame slots to the outgoing overflow area.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- `00204.c` still fails the selected proof, but the HFA long-double family now
  prints distinct second lanes such as `34.1,34.4`, `33.1,33.3`,
  `32.1,32.2`, and `31.1,31.1`; the earlier first-lane repetition is gone.
- The remaining HFA double/float corruption is an overflow-only family: the
  first two HFA aggregates are correct, then values copied from caller-side
  frame slots into the outgoing stack area become zero, NaN, or large garbage.
  Example current output includes `HFA double: 24.1,24.4 24.1,24.4 0.0,0.0`
  and `HFA float: 14.1,14.4 14.1,14.4 -0.0,-107374184.0`.
- The caller stack setup shape is ABI-plausible (`x16 = sp - size`, packed
  4- or 8-byte stores, then `sub sp, sp, #size`); the bad data is already in
  the selected source frame slots. Start the next slice at prepared
  `arg.source_selection=frame_slot_value` for HFA float/double overflow lanes,
  not at the F128 register-save extraction path.
- There is still a separate stdarg aggregate text mismatch in `00204.c`.
- Do not repair this by size/align guessing or by weakening expectations; that
  would be an ABI overfit risk for non-HFA scalar or aggregate cases.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$' > test_after.log 2>&1
```

Result: exit code 8. The selected build completed successfully, and CTest ran
the contained 26-test subset. 25/26 selected tests passed. The only selected
failure remains `c_testsuite_aarch64_backend_src_00204_c`; guard cases
`c_testsuite_aarch64_backend_src_00032_c` and
`c_testsuite_aarch64_backend_src_00182_c` passed, as did
`backend_prepare_frame_stack_call_contract` and the selected AArch64 route
tests, including `backend_codegen_route_aarch64_hfa_result_home_publication_contract`
and `backend_codegen_route_aarch64_f128_hfa_global_payload_call_boundary`.
`00204.c` still fails with remaining stdarg aggregate text mismatch and HFA
float/double overflow-source corruption; the long-double HFA lane-repetition
family is repaired.

Canonical executor proof log: `test_after.log`.
