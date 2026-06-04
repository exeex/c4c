Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 trace for the remaining `00204.c` long-double/F128 HFA
corruption after direct F128 producer loads were repaired.

- Reproduced `c_testsuite_aarch64_backend_src_00204_c` and confirmed direct
  argument paths are still good. Direct long-double returns now load initialized
  payloads but still show lane-selection issues for `fr_hfa32` through
  `fr_hfa34` in this workspace (`32.2 32.2`, `33.3 33.3`, `34.4 34.4`).
- Dumped semantic BIR, prepared BIR, and emitted AArch64 for `myprintf` and
  `stdarg`.
- Identified the next first wrong ABI fact for the variadic long-double HFA
  family: the BIR `llvm.va_arg.aggregate` classifier records F32/F64 HFA
  payloads with `va_arg_hfa_lane_count` and `va_arg_hfa_lane_size_bytes`, so
  prealloc builds `source_class=register_save_area` plans for those helpers.
  It does not record equivalent metadata for F128/long-double HFA payloads,
  so prealloc correctly sees the F128 aggregate helpers as plain
  `source_class=overflow_arg_area`.
- The direct cause is upstream of the owned prealloc/codegen files:
  `src/backend/bir/lir_to_bir/calling.cpp::va_arg_hfa_lane_size_bytes` accepts
  only F32 and F64, and `collect_va_arg_hfa_payload_lanes` rejects F128 scalar
  lanes. The emitted F128 blocks therefore fall back to generic overflow-area
  copying and never consume the caller-populated `q0`-`q7` register-save
  payloads.
- Left implementation unchanged because `src/backend/bir/lir_to_bir/calling.cpp`
  is outside this packet's owned files, and inferring F128 HFA from only
  size/alignment inside prealloc would risk misclassifying ordinary 16-byte
  aligned aggregates as HFA.
- No broad c-testsuite expectations or `00204.c` dump expectations were
  changed.

## Suggested Next

Delegate a narrow upstream BIR metadata packet to add AArch64 F128 HFA lane
recognition for `llvm.va_arg.aggregate` payloads in
`src/backend/bir/lir_to_bir/calling.cpp`, with focused coverage proving
`struct { long double ... }` va_arg helpers carry `va_arg_hfa_lane_count` and
lane size 16 before prealloc consumes them.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Direct non-variadic `fr_hfa31` through `fr_hfa34` now load initialized global
  F128 payloads before sret publication, but `00204.c` still fails in the
  selected proof.
- The remaining variadic long-double HFA output is no longer explained by a
  missing prepared-addressing fact for the direct F128 return producers. The
  visible variadic failure is an upstream metadata gap: F128 va_arg aggregate
  helpers lack HFA lane metadata and are planned as overflow-only.
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

Result: exit code 8. The selected build completed with no work to do, and
CTest ran the contained 26-test subset. 25/26 selected tests passed. The only
selected failure remains `c_testsuite_aarch64_backend_src_00204_c`; guard cases
`c_testsuite_aarch64_backend_src_00032_c` and
`c_testsuite_aarch64_backend_src_00182_c` passed, as did
`backend_prepare_frame_stack_call_contract` and the selected AArch64 route
tests. `00204.c` still fails in the long-double/F128 HFA variadic family, with
additional float/double variadic HFA corruption visible after the register-save
area paths spill/overflow.

Canonical executor proof log: `test_after.log`.
