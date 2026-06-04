Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Traced a Step 5 F128 transport gap while chasing the remaining `fr_hfa31`
through `fr_hfa34` long-double HFA return payloads, but the full `00204.c`
runtime repair is still blocked.

- Added a focused AArch64 dispatch contract for an F128 `LoadGlobalInst` whose
  prepared carrier is a q-register and whose prepared memory operand is a
  direct global symbol.
- Repaired F128 transport admission so global F128 loads can produce selected
  `F128TransportRecord` load-from-memory facts instead of falling through to
  the generic scalar/global-load route first.
- Moved F128 transport dispatch ahead of generic `LoadGlobalInst` lowering so
  prepared q-register F128 global-load carriers get first ownership.
- No broad c-testsuite expectations or `00204.c` dump expectations were
  changed.
- The delegated proof still fails only
  `c_testsuite_aarch64_backend_src_00204_c`. Generated AArch64 for
  `fr_hfa31` through `fr_hfa34` still stores an uninitialized `q13` stack slot
  into the `x8` sret destination, so the repaired F128 global-load fact is not
  the root repair for these callees.

## Suggested Next

Take the next focused packet at the producer side of the remaining F128 HFA
`00204.c` runtime path: dump/trace BIR, prepared BIR, and MIR for
`fr_hfa31` through `fr_hfa34` to identify why their global aggregate payload
does not become an F128 transport source before the sret copy. The current
evidence points past plain F128 `LoadGlobalInst` dispatch and toward
aggregate-copy/source-publication or local F128 `StoreLocal` transport
ownership.

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

Result: exit code 8. All selected backend route tests pass, including
`backend_codegen_route_aarch64_hfa_result_home_publication_contract`,
`backend_codegen_route_aarch64_hfa_global_payload_return`,
`backend_codegen_route_aarch64_f128_hfa_global_payload_call_boundary`, and the
prior SRET/large-stack publication probes. `backend_prepare_frame_stack_call_contract`
passes, and guard cases `00032.c`/`00182.c` pass. The selected proof is back to
the contained 26-test shape; the only selected failure remains
`c_testsuite_aarch64_backend_src_00204_c`. In that failure, expected
long-double HFA returns under `HFA long double:` include `34.1,34.4`,
`33.1,33.3`, `32.1,32.2`, and `31.1,31.1`; the actual corresponding block is
still all `0.0,0.0`. Full `00204.c` does not pass.

Also ran the direct
`build/tests/backend/mir/backend_aarch64_instruction_dispatch_test` binary
while checking the new focused contract. It reached the new F128 global-load
case, then failed later on an existing F64 global-readback selected-value test;
that direct binary is outside the delegated proof regex and was not used as
acceptance proof.

Canonical executor proof log: `test_after.log`.
