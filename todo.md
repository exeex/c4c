Status: Active
Source Idea Path: ideas/open/322_aarch64_va_start_destination_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 reran the focused proof and classified the post-Step-2 residual for idea
322.

The original `va_start` destination-materialization fault is repaired in the
fresh generated representative. In
`build/c_testsuite_aarch64_backend/src/00204.c.s`, `myprintf` now materializes
the local `va_list` object before field stores:

```asm
add x21, sp, #816
add x9, sp, #816
str x9, [x21]
add x9, sp, #680
str x9, [x21, #8]
add x9, sp, #816
str x9, [x21, #16]
str w9, [x21, #24]
str w9, [x21, #28]
```

No raw `va.arg.aggregate*` helper text appears in the generated assembly.

The remaining `c_testsuite_aarch64_backend_src_00204_c` failure is outside idea
322. The program now reaches the scalar and string varargs, then prints corrupt
long-double/floating output and finally exits with `Segmentation fault`. The
post-repair first bad fact is no longer an unmaterialized `va_start`
destination register; it is later AArch64 vararg source/progression or
floating/long-double value transport corruption while consuming the initialized
`va_list`.

## Suggested Next

Ask the plan owner to close idea 322 and create or activate the next owner for
the later AArch64 vararg consumption residual: aggregate/floating/long-double
`va_arg` source selection and progression after `va_start` has initialized a
real local `va_list` object.

## Watchouts

- The remaining `c_testsuite_aarch64_backend_src_00204_c` failure is still a
  runtime segfault, but it has advanced past the original `va_start`
  destination address fault. The stdout in `test_after.log` prints all
  scalar/string arguments and then emits corrupt long-double/floating values,
  including huge decimal values and `-nan 0.0`, before the segfault.
- Generated `myprintf` now initializes `x21` with `add x21, sp, #816` before
  all `str ... [x21]` `va_start` field stores. Do not classify the residual as
  idea 322 unless that materialized destination sequence disappears or is shown
  to address the wrong writable object.
- The later consumer code repeatedly loads and advances `[x21]` after the
  repaired initialization, so the next owner should inspect AArch64 `va_arg`
  source selection/progression for floating, long-double, and aggregate
  consumers rather than the destination address publication contract.
- `rg 'va\\.arg\\.aggregate' build/c_testsuite_aarch64_backend/src/00204.c.s`
  returns no matches after this packet.
- Do not reopen F128 transport addressability; that owner is closed.
- Do not reopen aggregate `va_arg` helper lowering; that owner is closed.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, `x21`, one local
  variable, one frame slot, one offset, or one emitted instruction sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Preserve the repairs from ideas 314, 315, 317, 318, 319, 320, and 321.
- Keep global initializer emission and later runtime mismatches separate unless
  they become the first bad fact after `va_start` destination publication is
  repaired.

## Proof

The delegated focused proof was rerun to refresh the canonical proof log at
`test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build succeeded. CTest ran 11 tests: 10 passed and 1 failed. The only
failure is `c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO] exit=Segmentation fault`. The focused internal backend
coverage in the subset passes. The remaining external failure is classified as
outside idea 322 because `va_start` field stores now use a materialized local
`va_list` destination and the failure occurs later while consuming varargs.
