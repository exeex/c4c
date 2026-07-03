Status: Active
Source Idea Path: ideas/open/562_prepared_object_data_zero_fill_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Zero-Fill Contract Coverage

# Current Packet

## Just Finished

Completed plan Step 2 coverage plus the minimal producer repair needed to make
that coverage pass.

Changed files:

- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `src/backend/prealloc/object_data.cpp`

Work completed:

- Added focused prepared object-data coverage for the actual lowered
  no-explicit-initializer const pointer-element aggregate shape: integer-array
  layout authority plus zero pointer initializer elements. The test asserts the
  producer facts directly: no emitted bytes, `requires_zero_fill`,
  `has_zero_fill`, and `zero_fill_byte_count == object_size_bytes`.
- Updated `populate_prepared_object_data_plans(...)` so eligible globals with
  no explicit initializer bytes and no initializer symbol publish full-extent
  BSS zero-fill authority instead of `unsupported_but_coherent`, including
  all-zero initializer element lists that cannot be byte-materialized by the
  existing scalar-byte helper.
- Preserved fail-closed handling for extern declarations, missing labels,
  missing extents, thread-local globals, GOT-required globals, and other
  non-materialized initializer shapes.

Step 3 repair is no longer a separate necessary packet for this producer
boundary; the corrected semantic repair was folded into Step 2 and now clears
the representative.

## Suggested Next

Ask the plan owner to evaluate lifecycle closure for this source idea. The
focused backend subset is green and the representative `src/20000412-1.c`
allowlist probe now passes.

## Watchouts

- This slice did not touch RV64 object emission, expectations, unsupported
  markers, allowlists, or pass/fail accounting.
- The repair is semantic: it keys on implicit-zero initializer state and normal
  object publication identity/extent, including all-zero initializer elements;
  it does not key on `src/20000412-1.c`, object label id 2, or a 1656-byte
  size.
- No downstream owner appeared in the representative probe.

## Proof

Ran delegated backend proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: passed. Canonical proof log: `test_after.log`.

Ran representative allowlist proof:

```sh
mkdir -p build/agent_state && printf '%s\n' src/20000412-1.c > build/agent_state/562_step2_zero_fill_after.allowlist && ALLOWLIST=build/agent_state/562_step2_zero_fill_after.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/562_step2_zero_fill_after.log 2>&1
```

Result: passed (`total=1 passed=1 failed=0`). Representative log:
`build/agent_state/562_step2_zero_fill_after.log`.
