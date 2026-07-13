# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 4.5
Current Step Title: Complete remaining functions, signatures, CFG and local objects

## Just Finished

- Completed Plan Step 4.4 by receiving authoritative scalar integer `LirRet`
  values into the existing typed `ReturnTerm` without adding schema or a
  parallel value registry.
- Exact native immediates become source-less typed ordinary constants; native
  current-function `LirValueId` returns reuse the existing Load/GEP-capable
  source-value registry and exact function signature type.
- Preserved valueless structured-void returns and added Raw/Canonical coverage
  for misleading displays, signature agreement, builder return guards, and
  transactional raw/malformed/unsupported/missing/extra/range/SSA failures.

## Suggested Next

- Have the supervisor select the first bounded Step 4.5 function/signature/CFG
  or local-object packet. Step 4.5 is a multi-family lifecycle boundary and
  should not be dispatched as one implicit implementation slice.

## Watchouts

- Return immediates intentionally have no source ID; SSA returns must retain
  the exact existing current-function result `ValueId` and source lookup.
- Noninteger/raw returns remain fail-closed. Do not use return completion to
  broaden into comparison/control receipt or parse display/type spelling.
- Preserve the array-address frontier at its later comparison/control row while
  Step 4.5 packet selection isolates function, CFG, and local-object ownership.

## Proof

- Fresh `cmake --build --preset default` completed successfully.
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$'
  --output-on-failure` passed 1/1.
- Exact `--dump-bir` observations now publish semantic Raw BIR for
  `aarch64_return_zero_smoke.c`, `lir_identity_global_store.c`, and
  `global_load.c`; `lir_identity_global_array_address.c` still stops at the
  later `UnsupportedOrdinaryInstruction` comparison/control row.
- `ctest --test-dir build -j --output-on-failure > test_after.log` passed
  3033/3033. The monotonic regression guard against `test_before.log` passed
  with delta `passed=0 failed=0` and no new over-30-second tests.
