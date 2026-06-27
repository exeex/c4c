Status: Active
Source Idea Path: ideas/open/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair The First Runtime Root Cause

# Current Packet

## Just Finished

Step 2 repaired the first runtime root cause: RV64 object lowering now
materializes prepared frame-slot address facts when a pointer-valued
`StoreLocalInst` source is the address of a local/frame slot.

Artifacts:

- `build/agent_state/402_step2_frame_slot_address_runtime/representatives.runner.log`
- `build/agent_state/402_step2_frame_slot_address_runtime/20070212-2/`
- `build/agent_state/402_step2_frame_slot_address_runtime/20000113-1/`

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Implementation:

- Added a generic helper that finds a unique current-instruction
  `PreparedAddressMaterializationKind::FrameSlot` for a pointer-typed named
  store source and materializes it as `addi rd, sp, offset`.
- Threaded the prepared block label and instruction index into
  `fragment_for_prepared_store_local(...)` so local stores can use the same
  address-materialization authority already published by the producer.
- The fallback path still uses normal value-register movement when no complete
  frame-slot address materialization exists.
- No filename/function-specific handling, expectation weakening, or RV64
  reconstruction from source names was added.

Focused test coverage:

- Added `make_prepared_frame_slot_address_local_store_module()` and
  `builds_prepared_frame_slot_address_local_store_object()`.
- The fixture gives the pointer address value a stale-looking register home
  (`s1`) but also publishes a prepared frame-slot address materialization. The
  assertion requires `addi t1, sp, 24` and `sd t1, 8(sp)`, and rejects the old
  stale `mv t1, s1` route.

Representative result:

- `20070212-2.c` now passes the clang-vs-c4c qemu comparison:
  `clang_qemu_status=0`, `c4c_qemu_status=0`, `prepared_status=0`,
  `c4c_bin_objdump_status=0`.
- Fresh c4c object code for `f` materializes the local addresses before storing
  `%lv.f1`: `mv t1, sp` for `&i1` at offset 0 and `addi t1, sp, 0x4` for
  `&j1`, then `sd t1, 0x8(sp)` and the later indirect `lw` succeeds.
- `20000113-1.c` remains the distinct residual:
  `clang_qemu_status=0`, `c4c_qemu_status=134`, with
  `[RV64_BACKEND_RUNTIME_MISMATCH]` / `Subprocess aborted`.

## Suggested Next

Execute Step 3 proof/classification for active 402: confirm the repaired
`20070212-2.c` family remains passing and classify/route the remaining
`20000113-1.c` abort residual as the next distinct runtime family or split.

## Watchouts

- Do not claim compile-only proof for 402; these cases require clang-vs-c4c
  qemu runtime comparison.
- Keep `20000113-1.c` out of this repair; it still appears to be an RV64 object
  value-scheduling/control-flow issue around RHS bitfield/boolean
  materialization, not the frame-slot local-address family.
- The new store-source address materialization intentionally fails closed for
  non-pointer values, non-frame-slot materializations, TLS/non-default address
  spaces, duplicate facts, missing frame slots, and out-of-range offsets.

## Proof

Exact delegated proof command run:

```sh
cmake --build --preset default &&
ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' &&
mkdir -p build/agent_state/402_step2_frame_slot_address_runtime &&
printf 'src/20070212-2.c\nsrc/20000113-1.c\n' > build/agent_state/402_step2_frame_slot_address_runtime/representatives.allowlist &&
(set +e; ALLOWLIST=build/agent_state/402_step2_frame_slot_address_runtime/representatives.allowlist VERBOSE_FAILURES=1 CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/402_step2_frame_slot_address_runtime/representatives.runner.log 2>&1; printf 'runner_status=%s\n' "$?" > build/agent_state/402_step2_frame_slot_address_runtime/representatives.runner.status) &&
for case in 20070212-2 20000113-1; do
  dir="build/agent_state/402_step2_frame_slot_address_runtime/${case}";
  work="build/rv64_gcc_c_torture_backend/src_${case}.c";
  src="tests/c/external/gcc_torture/src/${case}.c";
  mkdir -p "$dir";
  cp "$work/case.log" "$dir/case.log" 2>/dev/null || true;
  (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$dir/prepared.bir.txt" 2> "$dir/prepared.bir.err"; printf 'prepared_status=%s\n' "$?" > "$dir/prepared.bir.status");
  (llvm-objdump -d "$work/c4c.bin" > "$dir/c4c.bin.objdump.txt" 2> "$dir/c4c.bin.objdump.err"; printf 'c4c_bin_objdump_status=%s\n' "$?" > "$dir/c4c.bin.objdump.status");
  (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/clang.bin" > "$dir/clang.qemu.out" 2> "$dir/clang.qemu.err"; printf 'clang_qemu_status=%s\n' "$?" > "$dir/clang.qemu.status");
  (qemu-riscv64 -L /usr/riscv64-linux-gnu "$work/c4c.bin" > "$dir/c4c.qemu.out" 2> "$dir/c4c.qemu.err"; printf 'c4c_qemu_status=%s\n' "$?" > "$dir/c4c.qemu.status");
  cat "$dir"/*.status > "$dir/status.summary";
  rg -n 'address_materialization|lv\.param|lv\.f1|addi\s+[^,]+,\s*sp|sd|ld|lw|sw|SIGSEGV|Subprocess aborted|RV64_BACKEND_RUNTIME_MISMATCH|abort|foobar|logic_rhs' "$dir/case.log" "$dir/prepared.bir.txt" "$dir/prepared.bir.err" "$dir/c4c.bin.objdump.txt" "$dir/c4c.qemu.err" > "$dir/triage_hits.txt" 2>/dev/null || true;
done &&
cat build/agent_state/402_step2_frame_slot_address_runtime/representatives.runner.status build/agent_state/402_step2_frame_slot_address_runtime/20070212-2/status.summary build/agent_state/402_step2_frame_slot_address_runtime/20000113-1/status.summary &&
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- Focused `backend_riscv_object_emission` test passed.
- `representatives.runner.status`: `runner_status=1`, because
  `20000113-1.c` still mismatches; runner summary is `total=2 passed=1
  failed=1`.
- `20070212-2.c`: `prepared_status=0`, `c4c_bin_objdump_status=0`,
  `clang_qemu_status=0`, `c4c_qemu_status=0`.
- `20000113-1.c`: `prepared_status=0`, `c4c_bin_objdump_status=0`,
  `clang_qemu_status=0`, `c4c_qemu_status=134`.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
