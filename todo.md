Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 decomposed SRET source-publication repair packet for the next
remaining AArch64 `00204.c` ABI family after direct `Arguments:` matched.

Repaired the localized return/sret fact:

- AArch64 store-local source publication now recognizes a completed same-block
  scalar `LoadLocal` producer whose BIR address is a global symbol and emits
  `adrp`/`add`/scalar `ldr` before the original store publishes the byte to the
  temporary stack home.
- The repaired `fr_s1` path in `00204.c` now materializes `s1` before the stack
  byte and hidden `x8` copy: `adrp x9, s1`, `add x9, x9, :lo12:s1`,
  `ldrb w13, [x9]`, `strb w13, [sp, #1]`, `ldrb w13, [sp, #1]`,
  `strb w13, [x8]`.
- Added focused route case
  `backend_codegen_route_aarch64_sret_global_scalar_source_publication` so the
  repaired behavior is asserted directly and the stale no-source-load behavior
  is not codified.
- No broad c-testsuite expectations were changed and no `00204.c` special case
  was added.

## Suggested Next

Repair the next remaining AArch64 `00204.c` ABI family exposed by the proof:
large aggregate/byval stack publications still emit byte stores such as
`strb w9, [sp, #8112]`, which the AArch64 assembler rejects because the
immediate is outside the byte-store addressing range. Route this through the
existing large-offset materialization form instead of weakening expectations.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Small non-HFA aggregate return classification may still be modeled as sret;
  do not paper over that with expectation rewrites.
- HFA return output is also still suspect, but the current selected proof stops
  first on large-offset stack-store encoding in `00204.c`.
- The new SRET route case asserts the global materialization snippets and the
  stack/`x8` publication snippets; it does not rely on `00204.c` naming.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$' > test_after.log 2>&1
```

Result: exit code 8. All selected backend route tests pass, including the new
`backend_codegen_route_aarch64_sret_global_scalar_source_publication` probe.
`backend_prepare_frame_stack_call_contract` passes, and guard cases
`00032.c`/`00182.c` pass. The only selected failure remains
`c_testsuite_aarch64_backend_src_00204_c`, now blocked by assembler range
errors on large stack byte stores beginning at `strb w9, [sp, #8112]`.
Canonical executor proof log: `test_after.log`.
