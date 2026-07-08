Status: Active
Source Idea Path: ideas/open/602_bir_local_memory_load_semantics.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Local-Memory Load Production

# Current Packet

## Just Finished

Completed Step 3: repaired one narrow BIR load producer path in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp`. When a semantic
`load ptr, ptr @global_pointer` succeeds but does not publish a known pointee
or global alias, `lower_memory_load_inst(...)` now records the loaded SSA value
as an opaque runtime pointer in `pointer_value_addresses_`. That gives a
follow-on ordinary scalar dereference a real BIR memory source instead of
falling through to the old `load local-memory semantic family` stop.

Changed files:

- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `todo.md`

Representative load row results from the delegated 12-row proof:

- `src/20000706-4.c` old first owner was `bar` failing in `load local-memory
  semantic family`; new owner is `[RV64_BACKEND_RUNTIME_MISMATCH]` with
  `c4c_exit=Segmentation fault`. This row now clears the old BIR producer stop.
- `src/20010129-1.c` old first owner was `foo` failing in `load local-memory
  semantic family`; new owner is prepared/RV64 `unsupported_call_abi`. This row
  now clears the old BIR producer stop.
- `src/20041124-1.c`, `src/20011008-3.c`, and `src/920625-1.c` still fail in
  `load local-memory semantic family`. They remain aggregate/member,
  va_arg/byval, or complex pointer-load breadth work, not proof rows for this
  narrow scalar runtime-pointer repair.

Guard row results:

- `src/20010605-2.c` stayed in `store local-memory semantic family`.
- `src/20030717-1.c` stayed in `gep local-memory semantic family`.
- `src/20180921-1.c` stayed in `alloca local-memory semantic family`.
- `src/20000217-1.c` stayed at prepared move-bundle classification.
- `src/20021204-1.c` stayed at RV64 `unsupported_local_memory_access`.
- `src/20030910-1.c` stayed at `unsupported_terminator_fragment`.
- `src/20000706-1.c` stayed at `unsupported_move_bundle_target_shape` /
  stack-offset publication rejection.

## Suggested Next

Proceed to Step 4 with a same-family breadth packet. Use the current 12-row
set plus additional local-memory load rows that exercise opaque runtime pointer
loads, and keep aggregate/member and va_arg rows separated unless the packet
explicitly owns those subshapes.

## Watchouts

- Keep named cases as probes only, not match keys.
- `src/20000706-4.c` now reaches runtime mismatch. Do not treat that as a
  runtime/accounting fix in this lifecycle; it is only proof that the old BIR
  load producer stop cleared.
- `src/20010129-1.c` now reaches prepared/RV64 `unsupported_call_abi`; that is
  downstream of semantic BIR production and outside this packet.
- The remaining selected load rows still need later load producer work for
  aggregate/member, va_arg/byval, or complex pointer shapes.
- Do not weaken store/GEP/alloca/prepared/RV64 guard ownership to make the
  12-row total pass count improve.

## Proof

Build proof passed:

```sh
cmake --build --preset default
```

Delegated temporary 12-row RV64 gcc_torture backend proof was run and saved in
`test_after.log`:

```sh
tmp=$(mktemp /tmp/c4c-602-step3-allowlist.XXXXXX)
printf '%s\n' \
  'src/20041124-1.c' 'src/20011008-3.c' 'src/20000706-4.c' \
  'src/20010129-1.c' 'src/920625-1.c' 'src/20010605-2.c' \
  'src/20030717-1.c' 'src/20180921-1.c' 'src/20000217-1.c' \
  'src/20021204-1.c' 'src/20030910-1.c' 'src/20000706-1.c' > "$tmp"
BUILD_DIR=build ALLOWLIST="$tmp" \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true
rm -f "$tmp"
rg 'total=12|src/20010129-1.c|src/20011008-3.c|src/20041124-1.c|src/20010605-2.c|store local-memory|gep local-memory|alloca local-memory|unsupported_terminator_fragment|unsupported_move_bundle_target_shape' \
  test_after.log build/rv64_gcc_c_torture_backend/*/case.log
```

Proof summary: `total=12 passed=0 failed=12`; two selected load rows progressed
past the old load-family producer stop, and the guard rows retained non-load or
downstream ownership.

Supervisor validation after the delegated packet:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

The backend subset passed `346/346`. The restored full RV64 gcc_torture
backend-object scan now reports `total=1467 passed=473 failed=994`, so the
ignored scan artifacts are back in full-scan state and the slice adds three
passing rows relative to the `470/1467` baseline.
