Status: Active
Source Idea Path: ideas/open/409_prepared_packed_fp128_global_initializer_admission.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Publish The First Supported Initializer Facts

# Current Packet

## Just Finished

Step 2 implemented the first producer-side packed aggregate global admission
slice.

Changed files:

- `src/backend/bir/lir_to_bir/global_initializers.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `todo.md`

Implementation:

- Added packed struct initializer body support to the aggregate initializer
  walker by accepting LLVM packed aggregate initializer delimiters `<{ ... }>`
  alongside the existing ordinary `{ ... }` form.
- Kept layout authority in the existing aggregate type/layout machinery. The
  change does not reconstruct packed layout in RV64 object emission and does
  not use filename-specific or struct-name-specific handling.
- Existing scalar and aggregate leaf lowering remains fail-closed; unsupported
  scalar/initializer forms still return `std::nullopt`.

Focused test coverage:

- Added `expect_admitted_packed_integer_aggregate_global()` in
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`.
- The test builds a semantic RV64 packed `%struct.PackedInts = type <{ i16,
  i32, i16 }>` with structured type metadata and initializer
  `%struct.PackedInts <{ i16 17, i32 287454020, i16 33 }>`.
- It verifies byte-addressable aggregate storage facts: `type=I8`,
  `size_bytes=8`, `align_bytes=1`, and the explicit integer initializer lanes.
  This proves the packed field extent is preserved rather than rounded to the
  ordinary non-packed struct layout.

Representative result for `20040709-2.c`:

- `c4cll_status=0`.
- `prepared_status=1`.
- The old pre-handoff global admission diagnostic is gone; the probe no longer
  reports the `bootstrap lir_to_bir only supports scalar integer/pointer
  globals, linear integer-array globals, and aggregate-backed globals with
  honest byte-address semantics right now` boundary.
- Fresh residual:
  `error: backend BIR dump requires semantic lir_to_bir lowering before the prepared handoff: semantic lir_to_bir failed outside the currently admitted capability buckets ... latest function failure: semantic lir_to_bir function 'fn1A' failed in scalar-binop semantic family`.
- The residual is not the packed integer aggregate global admission family.
  It is a later semantic function-lowering/scalar-binop boundary reached after
  global admission advances. The source output still includes the mixed packed
  `fp128` globals `%struct.W` through `%struct.Z`, but this proof did not stop
  on their global initializer admission path.

## Suggested Next

Run Step 3 evidence/classification for `20040709-2.c`: confirm the packed
global initializer boundary is cleared, classify the fresh `fn1A`
scalar-binop pre-handoff residual, and route it to the appropriate existing
semantic lowering owner or a new split if no current idea owns it.

## Watchouts

- Do not edit `src/backend/riscv/rv64/object_emission.cpp`; this packet is a
  producer/global admission repair.
- The current representative still cannot produce a prepared dump because of
  the later `fn1A` scalar-binop semantic lowering failure, so Step 3 should not
  claim prepared global-storage lines from `20040709-2.c` unless that later
  boundary is separately repaired.
- The focused unit test is the committed proof for integer-only packed
  aggregate global storage facts in this slice.
- No testcase-specific handling, expectation downgrade, or allowlist change was
  used.

## Proof

Focused test command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'
```

Result:

- Passed: `backend_lir_to_bir_notes`.

Exact delegated proof command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/409_step2_packed_integer_global && (build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/409_step2_packed_integer_global/20040709-2.out 2> build/agent_state/409_step2_packed_integer_global/20040709-2.err; printf 'c4cll_status=%s\n' "$?" > build/agent_state/409_step2_packed_integer_global/20040709-2.status) && (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/409_step2_packed_integer_global/20040709-2.prepared.txt 2> build/agent_state/409_step2_packed_integer_global/20040709-2.prepared.err; printf 'prepared_status=%s\n' "$?" > build/agent_state/409_step2_packed_integer_global/20040709-2.prepared.status) && cat build/agent_state/409_step2_packed_integer_global/20040709-2.status build/agent_state/409_step2_packed_integer_global/20040709-2.prepared.status && rg -n 'bootstrap global admission|only scalar integer/pointer globals|linear integer-array globals|aggregate-backed globals|honest byte-address semantics|unsupported|error|fatal|global|fp128|packed|<\{|%struct\.|zeroinitializer|initializer|align|prepared|handoff|store_global|load_global|base=global_symbol|global_storage' build/agent_state/409_step2_packed_integer_global/20040709-2.out build/agent_state/409_step2_packed_integer_global/20040709-2.err build/agent_state/409_step2_packed_integer_global/20040709-2.prepared.txt build/agent_state/409_step2_packed_integer_global/20040709-2.prepared.err || true && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `c4cll_status=0`.
- `prepared_status=1` with the fresh `fn1A` scalar-binop diagnostic above.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
