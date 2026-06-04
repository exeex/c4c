Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 F128/long-double HFA `llvm.va_arg.aggregate` metadata repair.

- Added explicit AArch64 F128 HFA lane recognition in
  `src/backend/bir/lir_to_bir/calling.cpp`: F128 scalar lanes are accepted,
  lane size is recorded as 16 bytes, and recursive HFA collection resolves
  child layouts through the active type-decl plus structured-layout tables
  instead of re-parsing child type text without context.
- Added proof-covered coverage in
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp` that
  lowers a real AArch64 LIR `fp128` aggregate `va_arg`, asserts semantic
  `llvm.va_arg.aggregate` metadata before prealloc consumes it
  (`va_arg_hfa_lane_count=2`, `va_arg_hfa_lane_size_bytes=16`), and asserts a
  same-width non-floating aggregate does not gain HFA lane facts.
- Verified the real `00204.c` AArch64 prepared dump for `myprintf` now plans
  F128 HFA aggregate helpers from `source_class=register_save_area` with
  `register_save_lane_size=16` for 1-4 lanes instead of the prior
  `overflow_arg_area` fallback.
- No broad c-testsuite expectations or `00204.c` dump expectations were
  changed.

## Suggested Next

Delegate the next narrow AArch64 variadic HFA packet to repair register-save
lane extraction/overflow fallback behavior now that F128 `va_arg.aggregate`
helpers carry lane metadata. Start from the `00204.c` output families below and
the prepared plans for `myprintf` blocks 43, 46, 49, and 52.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- `00204.c` still fails the selected proof. The F128 variadic helpers are no
  longer planned as overflow-only, but runtime output now shows lane-selection
  corruption after register-save planning: long-double HFA variadic lines repeat
  the first lane (`34.1,34.1`, `33.1,33.1`, etc.) where the expected second
  lane is `34.4`, `33.3`, `32.2`, or `31.1`; the direct return family still
  shows `fr_hfa32` through `fr_hfa34` returning repeated second lanes
  (`32.2 32.2`, `33.3 33.3`, `34.4 34.4`).
- The same `00204.c` run also reports remaining stdarg aggregate text mismatch
  and severe HFA float/double corruption once the register-save paths are used,
  so the next slice should inspect downstream lane copy/addressing from
  register-save-area plans rather than BIR HFA classification.
- The F128 fallback in prepared-addressing is intentionally limited to
  `LoadLocalInst` with result type `F128`; widening it to scalar local loads or
  local stores regresses existing AArch64 byval global payload address output.
- Do not repair this in prealloc by size/align guessing; that would be an ABI
  overfit risk for non-HFA 16-byte aligned aggregates.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$' > test_after.log 2>&1
```

Result: exit code 8. The selected build completed successfully, and CTest ran
the contained 26-test subset. 25/26 selected tests passed. The only
selected failure remains `c_testsuite_aarch64_backend_src_00204_c`; guard cases
`c_testsuite_aarch64_backend_src_00032_c` and
`c_testsuite_aarch64_backend_src_00182_c` passed, as did
`backend_prepare_frame_stack_call_contract` and the selected AArch64 route
tests. `00204.c` still fails after F128 metadata repair, with remaining
stdarg aggregate text mismatch plus HFA long-double lane repetition and HFA
float/double corruption in the register-save-area path.

Canonical executor proof log: `test_after.log`.
