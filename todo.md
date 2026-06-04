Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed the Step 5 F128 HFA follow-up as a localization-only containment
packet after supervisor rejection.

- Re-tested the attempted F128/`long double` HFA return classification and
  Q-register result-bank route.
- Confirmed the first exposed wrong fact: enabling F128 HFA return
  classification makes `00204.c` function `ret` enter aggregate store
  local-memory lowering for F128 HFA call results.
- A small semantic repair was identified during tracing: local aggregate HFA
  store classification also needs to admit F128 lanes, and single-lane
  structured HFA call results need a lane publication/consumption contract.
- With that semantic admission in place, `00204.c` advances past BIR admission
  to a later prepared/MIR printer blocker, and the existing prepared `00204.c`
  dump contract changes its source id/slot snippets. Because this packet may
  not rewrite or downgrade that existing dump test, the F128 classification and
  focused proof slice were contained/reverted instead of accepted.
- No implementation/test changes remain from the rejected F128 slice; no broad
  c-testsuite expectations or dump expectations were changed.

## Suggested Next

Take a focused packet that first repairs/proves F128 HFA aggregate-store
publication independently of `00204.c`, then re-enable F128 HFA return
classification/result-bank authority with a non-brittle focused contract before
touching the existing `00204.c` prepared dump expectations.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Small non-HFA aggregate return classification may still be modeled as sret;
  do not paper over that with expectation rewrites.
- HFA return/output advanced: `fr_hfa11`, `fr_hfa12`, `fr_hfa13`,
  `fr_hfa14`, `fr_hfa21`, `fr_hfa22`, `fr_hfa23`, and `fr_hfa24` now print
  correctly in `00204.c`.
- The rejected F128 classification route is not accepted in the current tree.
  Re-enabling it exposes, in order, F128 HFA aggregate-store local-memory
  admission and then later prepared/MIR call-boundary publication issues.
- Do not rewrite the existing `00204.c` backend dump snippets merely to accept
  source-id churn from the F128 route; add a focused non-overfit contract first.
- The next remaining ABI family is still `fr_hfa31` through `fr_hfa34`
  long-double/F128 HFA return payloads, which print `0.0` in `00204.c` in the
  contained state.
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

Result: exit code 8. All selected backend route tests pass, including
`backend_codegen_route_aarch64_hfa_result_home_publication_contract`,
`backend_codegen_route_aarch64_hfa_global_payload_return`,
`backend_codegen_route_aarch64_f128_hfa_global_payload_call_boundary`, and the
prior SRET/large-stack publication probes. `backend_prepare_frame_stack_call_contract`
passes, and guard cases `00032.c`/`00182.c` pass. The selected proof is back to
the contained 26-test shape; the only selected failure remains
`c_testsuite_aarch64_backend_src_00204_c`.

Also ran:

```bash
ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' > test_cli_after.log 2>&1
```

Result: exit code 0. Both existing `00204.c` backend dump tests pass after
containment.
Canonical executor proof logs: `test_after.log`, `test_cli_after.log`.
