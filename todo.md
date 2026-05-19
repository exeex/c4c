Status: Active
Source Idea Path: ideas/open/322_aarch64_va_start_destination_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize Va Start Destination Address

# Current Packet

## Just Finished

Step 2 could not be implemented within the delegated owned-file boundary.

The required address materialization primitive is missing before
`src/backend/mir/aarch64/codegen/variadic.cpp` receives the printer record:
the `VariadicVaStartRecord::destination_va_list` fact remains only a register
value home, not a writable local `va_list` object address or a frame-slot base.

Evidence from the focused prepared dump for `myprintf`:

- `helper_operand kind=va_start block=0 inst=0 dst_va_list=%lv.ap:register:reg=x21`
  is the only destination fact copied into `VariadicVaStartRecord`.
- The call argument plan for `llvm.va_start.p0` likewise records
  `arg0 bank=gpr from=register:x21 to=x0`; it has no source frame slot or
  computed-address materialization fact.
- The local `va_list` storage is currently split across independent field
  locals such as `%lv.ap.0`, `%lv.ap.8`, and `%lv.ap.24`, whose frame slots
  are not contiguous in the prepared layout (`%lv.ap.0` at stack offset 24,
  `%lv.ap.8` at stack offset 352, `%lv.ap.24` at stack offset 520 in the
  representative dump).
- No `address_materialization` record exists for `%lv.ap`; only unrelated
  local addresses such as `%lv.s` are materialized.

Given those facts, `variadic.cpp` can either keep storing through an
uninitialized register or invent a testcase-shaped stack address. The general
repair needs preparation/address-materialization ownership to publish a real
contiguous writable `va_list` destination home, or to carry structured field
homes for `va_start`/`va_arg` lowering. That producer is outside this packet's
owned files.

## Suggested Next

Delegate a plan/lifecycle repair or a widened implementation packet that owns
the prepared variadic destination-home producer. The smallest coherent packet
should add a general AArch64 `va_start` destination publication fact before MIR
printing, either as a contiguous local `va_list` frame object address or as
structured per-field homes shared by `va_start` and later `va_arg` helpers.

## Watchouts

- Do not implement the repair in `variadic.cpp` by special-casing `x21`,
  `myprintf`, `00204.c`, or the observed `%lv.ap.*` stack offsets. The current
  printer record does not carry enough destination ownership to do this
  generally.
- A printer-only change that materializes `x21` from one observed frame offset
  would be testcase overfit because the split `%lv.ap.*` fields are not a
  contiguous `va_list` object in prepared state.
- `rg 'va\\.arg\\.aggregate' build/c_testsuite_aarch64_backend/src/00204.c.s`
  returns no matches after this packet.
- The external representative now reaches runtime, so the prior raw helper
  text assembler blocker is gone.
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

No code change was made, but the delegated focused proof was rerun to preserve
the canonical proof log at `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build succeeded. CTest ran 11 tests: 10 passed and 1 failed. The only
failure is `c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO] exit=Segmentation fault`. The blocker is the missing
prepared destination-home/address materialization primitive, not aggregate
`va_arg` helper text.
