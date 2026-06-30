Status: Active
Source Idea Path: ideas/open/429_rv64_pointer_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Lower Coherent Address Materialization

# Current Packet

## Just Finished

Completed Step 2, Lower Coherent Pointer Cast Movement.

Implemented a prepared-fact-driven RV64 object lowering route for coherent
scalar pointer cast movement in
`src/backend/mir/riscv/codegen/object_emission.cpp`:

- accepts `IntToPtr` from `i32`/`i64` to `ptr` only when the result has a plain
  prepared GPR register home;
- accepts `PtrToInt` from `ptr` to `i64` only when source and result have plain
  prepared GPR register homes;
- accepts coherent prepared rematerializable integer sources and literal
  integer operands without inventing object identity;
- rejects missing homes, stack result homes, pointer-base-plus-offset homes,
  unsupported widths such as `i16` and `ptrtoint` to `i32`, and global/address
  residual shapes by falling back to the existing unsupported diagnostics.

Focused tests in
`tests/backend/mir/backend_riscv_object_emission_test.cpp` cover accepted
`inttoptr`, accepted `ptrtoint`, literal/rematerialized integer sources, and
fail-closed missing-home, stack-home, pointer-base-plus-offset, and unsupported
width/form cases.

Step artifact: `build/agent_state/429_step2_pointer_cast_movement/summary.tsv`.

## Suggested Next

Execute Step 3: lower coherent address materialization only when prepared
frame-slot, pointer-base, symbol, or relocation facts explicitly authorize the
address; keep global memory/addressing residuals and incomplete provenance
facts fail-closed.

## Watchouts

- Do not infer object identity, provenance, relocation base, or addressability
  from integer shape, source filenames, raw BIR, or representative row text.
- Step 2 deliberately did not accept pointer-base-plus-offset homes, stack
  pointer-cast result publication, global symbol address materialization, or
  global load/store addressing.
- Keep `src/930930-1.c` stack-result/pointer-value memory residuals and
  `src/20041112-1.c` direct-global address residuals out of the pointer-cast
  packet unless Step 3 proves explicit address facts for them.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Step 2 implementation proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Proof log: `test_after.log`.
