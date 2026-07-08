Status: Active
Source Idea Path: ideas/open/602_bir_local_memory_load_semantics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Representative Load-Family Proof Rows

# Current Packet

## Just Finished

Completed Step 1: selected representative local-memory load proof rows and
adjacent guard rows from the current RV64 gcc_torture backend scan evidence.
No implementation, tests, expectations, unsupported markers, persistent
allowlists, runtime/timeout/accounting behavior, `plan.md`, source ideas, or
handoff docs were touched.

Load-family proof rows selected from the `82` local-memory load bucket:

- `src/20041124-1.c`: simple representative from the failure map; case log
  stops before prepared handoff with `semantic lir_to_bir` and
  `function 'main' failed in load local-memory semantic family`.
- `src/20011008-3.c`: non-`main` row; same producer stop with
  `function '__db_txnlist_lsnadd' failed in load local-memory semantic family`.
- `src/20000706-4.c`: nearby local-control-flow row; same producer stop with
  `function 'bar' failed in load local-memory semantic family`.
- `src/20010129-1.c`: small function probe; same producer stop with
  `function 'foo' failed in load local-memory semantic family`.
- `src/920625-1.c`: older gcc_torture row; same producer stop with
  `function 'main' failed in load local-memory semantic family`.

Adjacent guard rows selected so the load repair does not absorb other owners:

- `src/20010605-2.c`: store guard; `function 'main' failed in store
  local-memory semantic family`.
- `src/20030717-1.c`: GEP guard; `function 'bar' failed in gep local-memory
  semantic family`.
- `src/20180921-1.c`: alloca guard; `function 'aw' failed in alloca
  local-memory semantic family`.
- `src/20000217-1.c`: prepared/prealloc authority guard;
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
- `src/20021204-1.c`: prepared/RV64 local-memory consumption guard;
  `unsupported_local_memory_access` requiring prepared frame-slot or
  pointer-value base-plus-offset local memory addressing.
- `src/20030910-1.c`: RV64/MIR consumer guard; `unsupported_terminator_fragment`.
- `src/20000706-1.c`: RV64 move/select publication guard;
  `unsupported_move_bundle_target_shape` with `unsupported_source_stack_offset`.

## Suggested Next

Execute Step 2: trace the selected load rows through frontend, HIR/BIR
production, and the prepared/RV64 handoff to locate the missing BIR load fact.
Use the guard rows only to rule out store, GEP, alloca, prepared authority, or
RV64 consumer ownership; do not repair those families in the load packet.

## Watchouts

- Keep named cases as probes only, not match keys.
- Do not route this through RV64 target inference, expectation edits,
  unsupported markers, allowlists, timeout/runtime handling, or accounting.
- The old load signature to clear is the first-owner producer stop:
  `[RV64_C4C_OBJ_COMPILE_FAIL]` with `backend object route requires semantic
  lir_to_bir lowering before the prepared object handoff` and
  `failed in load local-memory semantic family`.
- Guard rows must remain honestly classified unless a later lifecycle route
  owns them. In particular, store/GEP/alloca producer rows, prepared
  destination fan-in, prepared/RV64 local-memory access, and RV64/MIR consumer
  fragments are not proof of load-family progress.

## Proof

Evidence-selection proof and temporary-allowlist command were run; output is in
`test_after.log`.

```sh
tmp=$(mktemp /tmp/c4c-602-step1-allowlist.XXXXXX)
printf '%s\n' \
  'src/20041124-1.c' \
  'src/20011008-3.c' \
  'src/20000706-4.c' \
  'src/20010129-1.c' \
  'src/920625-1.c' \
  'src/20010605-2.c' \
  'src/20030717-1.c' \
  'src/20180921-1.c' \
  'src/20000217-1.c' \
  'src/20021204-1.c' \
  'src/20030910-1.c' \
  'src/20000706-1.c' > "$tmp"
BUILD_DIR=build ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh
rm -f "$tmp"
```

The temporary run reported `total=12 passed=0 failed=12`, which is expected
before implementation. The persistent
`tests/c/external/gcc_torture/allowlist.txt` was not edited.

Delegated proof command for the `todo.md` update:

```sh
rg 'local-memory load|load local-memory|local-memory|proof rows|guard rows|ALLOWLIST|check_progress_rv64_gcc_c_torture_backend' todo.md
```
