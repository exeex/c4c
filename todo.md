Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 decomposed large-offset stack byte-store encoding repair
packet for the next remaining AArch64 `00204.c` blocker.

Repaired the localized memory-lowering fact:

- Prepared AArch64 scalar stack stores now check whether a frame-slot store
  offset is encodable for the selected store width before printing direct
  `[sp, #offset]`.
- Large frame-slot scalar stores materialize `sp + offset` through an existing
  frame-slot address materialization path and store through the scratch address
  register, e.g. `movz x10, #8112`, `add x10, sp, x10`, `strb w9, [x10]`.
- Added focused route case
  `backend_codegen_route_aarch64_large_stack_global_byte_publication` so a
  large global-byte-to-stack publication asserts the materialized form and
  forbids the illegal direct `strb ..., [sp, #8112]` shape.
- No broad c-testsuite expectations were changed and no `00204.c` special case
  was added.

## Suggested Next

Repair the next remaining AArch64 `00204.c` ABI family exposed after the
assembler blocker is gone: the selected proof now reaches runtime mismatch,
with remaining corruption concentrated in HFA float/double payload output.
Trace HFA argument/return lane placement and variadic handoff rather than
weakening expectations.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Small non-HFA aggregate return classification may still be modeled as sret;
  do not paper over that with expectation rewrites.
- HFA return/output is still suspect; the current selected proof now gets past
  the large-offset byte-store assembler error and fails at `00204.c` runtime
  comparison.
- The new SRET route case asserts the global materialization snippets and the
  stack/`x8` publication snippets; it does not rely on `00204.c` naming.
- The new large-stack route case asserts a legal materialized byte-store form
  and forbids the direct large `[sp, #8112]` byte-store form.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$' > test_after.log 2>&1
```

Result: exit code 8. All selected backend route tests pass, including the new
`backend_codegen_route_aarch64_sret_global_scalar_source_publication` probe and
`backend_codegen_route_aarch64_large_stack_global_byte_publication`.
`backend_prepare_frame_stack_call_contract` passes, and guard cases
`00032.c`/`00182.c` pass. The only selected failure remains
`c_testsuite_aarch64_backend_src_00204_c`, now a runtime mismatch after the
large-offset byte-store assembler errors are gone.
Canonical executor proof log: `test_after.log`.
