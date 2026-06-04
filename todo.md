Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Traced Step 5 producer-side facts for the remaining `fr_hfa31` through
`fr_hfa34` long-double/F128 HFA return payloads and stopped at an ownership
boundary before changing implementation.

- Focused BIR for `fr_hfa31` through `fr_hfa34` shows the same producer shape:
  global aggregate F128 lanes are represented as `bir.load_local f128` with
  global-symbol addresses such as `addr hfa31`, followed by local stores and
  sret-pointer stores.
- Focused prepared BIR for `fr_hfa31` proves the first wrong producer-side
  fact: `prepared-addressing` has no memory access for the initial global-symbol
  load at `entry` `inst_index=0`; it starts at the local store at `inst_index=1`.
- Because F128 transport now owns F128 `LoadLocalInst` before ordinary memory
  lowering, the missing prepared memory access makes the global-symbol load
  handled without emitting the required `adrp`/`ldr q...` producer. The later
  local/sret stores then publish uninitialized `q13` through `x8`.
- The direct repair point is
  `src/backend/prealloc/stack_layout/coordinator.cpp` in the prepared-addressing
  publisher, specifically the direct symbol-backed `LoadLocalInst` access
  path. That file is not in this packet's owned-file list, so no implementation
  repair was made here.
- No broad c-testsuite expectations or `00204.c` dump expectations were
  changed.

## Suggested Next

Delegate a narrow prepared-addressing packet that owns
`src/backend/prealloc/stack_layout/coordinator.cpp`: add focused non-overfit
coverage proving a `LoadLocalInst` with an F128 result and global-symbol memory
address receives a `PreparedMemoryAccess`, then repair the direct
symbol-backed access publisher so F128 transport can emit the producer load
before local/sret publication.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Small non-HFA aggregate return classification may still be modeled as sret;
  do not paper over that with expectation rewrites.
- HFA return/output advanced: `fr_hfa11`, `fr_hfa12`, `fr_hfa13`,
  `fr_hfa14`, `fr_hfa21`, `fr_hfa22`, `fr_hfa23`, and `fr_hfa24` now print
  correctly in `00204.c`.
- The focused q-register stack publication contract is accepted in the current
  tree; it is not sufficient to make `00204.c` pass.
- The new F128 global-load transport contract compiles and is exercised by the
  selected proof, but it is not sufficient to make `00204.c` pass.
- Do not rewrite the existing `00204.c` backend dump snippets merely to accept
  source-id churn from the F128 route.
- The next remaining ABI family is still `fr_hfa31` through `fr_hfa34`
  long-double/F128 HFA return payloads, which print `0.0` in `00204.c` in the
  current proof.
- `fr_hfa31` through `fr_hfa34` still emit the same uninitialized-q-register
  sret shape after this packet: a stack slot is filled from `q13`, reloaded,
  then stored through `x8`.
- The first wrong fact is not in final AArch64 printing and not in the F128
  global `LoadGlobalInst` route: it is the absent `PreparedMemoryAccess` for
  global-symbol `LoadLocalInst` producers in prepared addressing.
- A candidate implementation file for the repair is
  `src/backend/prealloc/stack_layout/coordinator.cpp`; keep the next packet's
  ownership aligned before editing it.
- F128 `StoreLocal` is intercepted by the F128 transport owner before ordinary
  memory-store retargeting. If the next trace reaches local F128 aggregate
  stores after a call result, repair may need an owned memory-transport packet
  rather than more call-boundary-only changes.
- Variadic HFA output is still corrupt. Long double HFA variadic output remains
  all zero; float/double variadic output still corrupts later lane groups after
  the initial register-resident payloads.
- The new SRET route case asserts the global materialization snippets and the
  stack/`x8` publication snippets; it does not rely on `00204.c` naming.
- The new large-stack route case asserts a legal materialized byte-store form
  and forbids the direct large `[sp, #8112]` byte-store form.
- The new HFA return route case asserts global FP lane loads for float/double
  HFA returns; it does not depend on `00204.c` naming.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$' > test_after.log 2>&1
```

Result: exit code 8. The selected build completed with no rebuild work needed,
and CTest ran the contained 26-test subset. 25/26 selected tests passed. The
only selected failure remains `c_testsuite_aarch64_backend_src_00204_c`; guard
cases `c_testsuite_aarch64_backend_src_00032_c` and
`c_testsuite_aarch64_backend_src_00182_c` passed, as did the selected backend
route and `backend_prepare_frame_stack_call_contract` tests. The `00204.c`
output family remains the long-double/F128 HFA area: `fr_hfa31` through
`fr_hfa34` still lower to uninitialized `q13` stack publications into the
`x8` sret destination because the producer global-symbol F128 loads lack
prepared memory access facts.

Canonical executor proof log: `test_after.log`.
