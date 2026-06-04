Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed the Step 5 decomposed HFA return/consumption packet for the next
remaining AArch64 `00204.c` blocker after the global HFA return lane-source
repair.

Repaired the first wrong multi-lane HFA return/consumption fact:

- `fr_hfa23().a` and `fr_hfa24().a` now print `23.1` and `24.1` instead of
  `0.0`.
- The first wrong fact was caller-side result-home publication, not callee
  return publication: `fr_hfa23`/`fr_hfa24` already returned lanes in `d0..dN`,
  but the caller later reloaded lane 0 from the aggregate result home without
  ever publishing the ABI result register there.
- `PreparedCallResultPlan` now records lane-0 HFA aggregate results as sourced
  from the ABI result register when lane 0 aliases `call.result`.
- AArch64 after-call lowering now emits a synthetic stack result-home
  publication for stack-homed scalar call results when no explicit after-call
  move bundle publishes that result.
- Stack call-result publication now supports scalar FPR sources (`sN`/`dN`) in
  addition to the previous scalar GPR path.
- Added focused non-`00204.c` coverage:
  `backend_codegen_route_aarch64_hfa_result_home_publication_contract`, which
  runs the AArch64 call-boundary owner contract and asserts that a stack-homed
  HFA lane-0 call result without an explicit after-call move bundle stores ABI
  `d0` into the aggregate result home.
- No broad c-testsuite expectations were changed and no `00204.c` special case
  was added.

## Suggested Next

Repair the next remaining AArch64 `00204.c` ABI family exposed after float and
double HFA return/consumption now pass: long-double HFA return payloads still
print `0.0`, and variadic HFA register/overflow payload consumption remains
corrupt. Trace long-double HFA ABI classification/publication first, then
variadic HFA overflow lane progression; do not weaken expectations.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Small non-HFA aggregate return classification may still be modeled as sret;
  do not paper over that with expectation rewrites.
- HFA return/output advanced: `fr_hfa11`, `fr_hfa12`, `fr_hfa13`,
  `fr_hfa14`, `fr_hfa21`, `fr_hfa22`, `fr_hfa23`, and `fr_hfa24` now print
  correctly in `00204.c`.
- The next return mismatch is long-double HFA return output: `fr_hfa31`,
  `fr_hfa32`, `fr_hfa33`, and `fr_hfa34` still print `0.0`.
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
`backend_codegen_route_aarch64_hfa_result_home_publication_contract` focused
contract and `backend_codegen_route_aarch64_hfa_global_payload_return` probe
plus the prior SRET and large-stack publication probes.
`backend_prepare_frame_stack_call_contract` passes, and guard cases
`00032.c`/`00182.c` pass. The selected proof now runs 26 tests with one
additional passing focused test; the only selected failure remains
`c_testsuite_aarch64_backend_src_00204_c`, now advanced past `fr_hfa23().a` /
`fr_hfa24().a` and failing later long-double HFA return plus variadic payload
families.
Canonical executor proof log: `test_after.log`.
