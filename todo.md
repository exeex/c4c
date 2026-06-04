Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 decomposed HFA runtime payload packet for the next remaining
AArch64 `00204.c` blocker.

Repaired the first wrong HFA return payload fact:

- AArch64 store-local value publication for a prepared `LoadLocalInst` whose
  address is a global symbol now supports `F32`/`F64` scalar lanes and emits
  `ldr sN/dN, [xScratch]` after materializing the direct global address.
- This repairs the uninitialized HFA return-lane source seen in `fr_hfa11`,
  where BIR had `load_local float ..., addr hfa11` but assembly previously
  started by storing the uninitialized FPR home to the frame slot.
- Added focused route case
  `backend_codegen_route_aarch64_hfa_global_payload_return` so float and double
  HFA returns from global aggregate payloads assert direct global FP lane loads
  before return ABI publication.
- No broad c-testsuite expectations were changed and no `00204.c` special case
  was added.

## Suggested Next

Repair the next remaining AArch64 `00204.c` ABI family exposed after the
global HFA return lane-source repair: multi-lane HFA return/consumption still
corrupts later double/long-double cases, and variadic HFA register/overflow
payload consumption remains red after the now-correct early float HFA returns.
Trace return-lane stack-home assignment and variadic HFA overflow lane
progression rather than weakening expectations.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Small non-HFA aggregate return classification may still be modeled as sret;
  do not paper over that with expectation rewrites.
- HFA return/output advanced: `fr_hfa11`, `fr_hfa12`, `fr_hfa13`,
  `fr_hfa14`, `fr_hfa21`, and `fr_hfa22` now print correctly in `00204.c`.
  The next return mismatch is `fr_hfa23().a` / `fr_hfa24().a` printing `0.0`
  while later selected lanes survive.
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

Result: exit code 8. All selected backend route tests pass, including the new
`backend_codegen_route_aarch64_hfa_global_payload_return` probe plus the prior
SRET and large-stack publication probes. `backend_prepare_frame_stack_call_contract`
passes, and guard cases `00032.c`/`00182.c` pass. The only selected failure
remains `c_testsuite_aarch64_backend_src_00204_c`, now advanced past the first
global HFA float return source mismatch and failing later HFA return/variadic
payload families.
Canonical executor proof log: `test_after.log`.
