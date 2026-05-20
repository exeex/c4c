Status: Active
Source Idea Path: ideas/open/345_aarch64_scalar_select_result_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Scalar Select Result Publication

# Current Packet

## Just Finished

Completed `plan.md` Step 1 localization for
`c_testsuite_aarch64_backend_src_00143_c`.

First bad owner: `src/backend/mir/aarch64/codegen/alu.cpp`
`lower_scalar_select_publication`. It resolves the prepared scalar select
result home through `control_prepared_scalar_result_operand`, receives
`result_stack_offset_bytes`, computes the selected value into the result
register with `csel`, then discards the stack-home offset and only records the
emitted register in `BlockScalarLoweringState`. The modeled select-result home
is therefore never written before the next local-store consumer reloads it.

Key evidence:

- Semantic BIR and prepared BIR both model the same producer/consumer shape:
  `%t13.store0 = bir.select eq i64 %t12, 0, i16 0, %t13.elt0` immediately
  followed by `bir.store_local %lv.b.0, i16 %t13.store0`; the same shape exists
  for the `%t9.store*` initialization chain.
- Prepared storage assigns stack homes to the select results, e.g.
  `%t13.store0` has stack storage at offset 402 while the destination local
  `%lv.b.0` is addressed through frame slot #39. Prepared addressing records
  the store-local consumer at `block_1 inst_index=125` storing `%t13.store0`
  into frame slot #39.
- Generated AArch64 computes the fresh selected value in `w9` with
  `csel w9, w10, w9, eq`, but the immediately following consumer reloads
  `ldrh w9, [sp, #402]` and stores that reloaded value with
  `strh w9, [sp, #78]`. There is no intervening store of the `csel` result to
  `[sp, #402]`.
- The generic store-local publication boundary in
  `lower_store_local_value_publication` only handles byval frame-slot loads and
  wide loads from narrow local stores. It does not currently recognize a
  same-block scalar select producer as a value-publication case, so it does not
  compensate for the unpublished select-result home.

## Suggested Next

Delegate Step 2 to an executor: repair scalar select result publication
generally at `lower_scalar_select_publication` by publishing the fresh `csel`
register value to the modeled result home when the prepared result has stack
storage, or by adding the equivalent general store-local publication path for a
same-block scalar select producer. Add focused coverage for a short integer
select result feeding a local store or equivalent stack-homed consumer.

## Watchouts

- Do not special-case `00143`, `%t9.store0`, `%t13.store0`, `%lv.a.0`, one
  stack offset, one block number, one source line, or one emitted instruction
  string.
- Do not weaken expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Preserve the idea 344 pointer-local dereference repair and the Duff emission
  repairs from ideas 343 and 342.
- `lower_scalar_select_publication` currently discards
  `result_stack_offset_bytes`; any repair should be keyed on prepared storage
  metadata, not on temporary names or observed offsets.
- The AArch64 `--dump-mir` surface still reports the x86/debug contract stub
  for this target, so the useful evidence for this packet is semantic BIR,
  prepared BIR/storage/addressing, and generated AArch64.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c' > test_after.log 2>&1`

Result: build completed with `ninja: no work to do`; focused CTest still fails
as expected for this localization-only packet. `test_after.log` records
`c_testsuite_aarch64_backend_src_00143_c` failing with `[RUNTIME_NONZERO]`
`exit=1`.
