Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 prepared-addressing repair for direct symbol-backed F128
`LoadLocalInst` producers.

- Added focused coverage in `backend_prepare_frame_stack_call_contract` proving
  an F128 `LoadLocalInst` whose global symbol spelling is carried by the local
  load receives a `PreparedMemoryAccess` with `base=global_symbol`, size 16,
  align 16, and direct global materialization policy.
- Repaired `src/backend/prealloc/stack_layout/coordinator.cpp` so only F128
  direct local loads may opt into link-name-spelling global resolution from the
  local load symbol spelling. Non-F128 local loads and local stores retain the
  existing raw/no-id compatibility behavior; the byval global payload address
  route guard remains green.
- Verified focused prepared BIR for `fr_hfa31` now includes
  `access block=entry inst_index=0 base=global_symbol
  result=%t0.global.aggregate.load.0 symbol=hfa31`, before the frame-slot and
  `x8` sret publications.
- Verified focused AArch64 output for `fr_hfa31` through `fr_hfa34` now emits
  global `ldr q13, [x9]` producer loads before storing the F128 lanes through
  `x8`.
- No broad c-testsuite expectations or `00204.c` dump expectations were
  changed.

## Suggested Next

Delegate a narrow packet for the remaining `00204.c` runtime failure after the
direct F128 return producers were repaired: trace the still-corrupt long-double
HFA output in the caller/variadic paths and decide whether the next wrong fact
is F128 argument/result classification, variadic HFA transport, or another
publication route.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Direct non-variadic `fr_hfa31` through `fr_hfa34` now load initialized global
  F128 payloads before sret publication, but `00204.c` still fails in the
  selected proof.
- The remaining `00204.c` output is no longer explained by a missing
  prepared-addressing fact for the direct F128 return producers. The visible
  failing family is still long-double/F128 HFA output: direct return lines
  advance to initialized values, while later long-double HFA variadic/caller
  groups still print zero or corrupted payloads.
- The F128 fallback in prepared-addressing is intentionally limited to
  `LoadLocalInst` with result type `F128`; widening it to scalar local loads or
  local stores regresses existing AArch64 byval global payload address output.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$' > test_after.log 2>&1
```

Result: exit code 8. The selected build completed, and CTest ran the contained
26-test subset. 25/26 selected tests passed. The only selected failure remains
`c_testsuite_aarch64_backend_src_00204_c`; guard cases
`c_testsuite_aarch64_backend_src_00032_c` and
`c_testsuite_aarch64_backend_src_00182_c` passed, as did
`backend_prepare_frame_stack_call_contract` and the selected AArch64 route
tests. `00204.c` does not pass yet.

Canonical executor proof log: `test_after.log`.
