Status: Active
Source Idea Path: ideas/open/322_aarch64_va_start_destination_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize Va Start Destination Address

# Current Packet

## Just Finished

Step 2 implemented a general prepared AArch64 `va_start` destination address
publication contract.

The prepared variadic entry plan now publishes
`PreparedVariadicEntryHelperOperandHomes::destination_va_list_address` for
`VaStart` helpers. The fact is a real stack-slot home allocated from the
prepared variadic storage authority using the destination `va_list` value
identity and the AAPCS64 `va_list` layout size/alignment.

The AArch64 variadic record and printer now require that address fact. Before
emitting `va_start` field stores, `print_va_start_lowering_lines()` materializes
the published stack-slot address into the destination register, then stores
`overflow_arg_area`, `gp_register_save_area`, `fp_register_save_area`,
`gp_offset`, and `fp_offset` through that initialized base. This removes the
prior dependence on an uninitialized destination register.

Focused backend coverage now requires and observes the new contract in the
prepared helper homes, target instruction record selection, dispatch, and
machine-printer output. The representative assembly contains no raw
`va.arg.aggregate*` text and now shows the former destination register
materialized before the `va_start` stores, e.g. `add x21, sp, #816` before the
`str ... [x21, ...]` field writes in `build/c_testsuite_aarch64_backend/src/00204.c.s`.

## Suggested Next

Run Step 3 as a small coverage-hardening packet if the supervisor wants more
local prepared-BIR assertions for the new `dst_va_list_addr` printed fact;
otherwise classify the remaining external `00204.c` runtime segfault as the
next first bad fact outside this step.

## Watchouts

- The remaining `c_testsuite_aarch64_backend_src_00204_c` failure is still a
  runtime segfault, but it has advanced past the original `va_start` destination
  address fault. The program prints all scalar/string arguments and then emits
  corrupt long-double/floating output before the segfault.
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

The delegated focused proof was rerun to preserve the canonical proof log at
`test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build succeeded. CTest ran 11 tests: 10 passed and 1 failed. The only
failure is `c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO] exit=Segmentation fault`. The focused internal backend
coverage in the subset passes; the remaining external failure is now later
runtime corruption after `va_start` destination address materialization.
