Status: Active
Source Idea Path: ideas/open/382_rv64_object_route_prepared_local_memory_addressing.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Rerun Representative And Route Next Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by rerunning the supervisor-selected
`src/20030914-2.c` RV64 object-route representative and classifying the next
boundary from `prepared.log`.

The representative still fails with
`unsupported_local_memory_access: RV64 object route requires prepared
frame-slot or pointer-value base-plus-offset local memory addressing`, but the
first concrete failing prepared shape is no longer the byval parameter copy in
`f`. `prepared.log` shows `f` entry insts `0,2,...,34` as prepared
`base=pointer_value` loads from `%p.pa+0..68`, paired with prepared
`base=frame_slot` stores for insts `1,3,...,35`.

The next exposed shape is in function `main`, block `entry`, starting at inst
`0`: `%t0.global.aggregate.load.0 = bir.load_local i32 %t0.0, addr gs`.
The same aggregate-global lane pattern repeats at offsets `4..68`. The
prepared-addressing section has frame-slot store entries for `main` insts
`1,3,...,35`, plus address materialization for `%t0` at inst `36`, but it has
no frame-slot or pointer-value access entry for the global-source
`load_local ... addr gs` insts themselves.

This boundary is a global aggregate/data materialization owner, not more
prepared frame-slot or pointer-value local-memory work under this idea. The
active idea lists unrelated global/data-section materialization as out of
scope; no matching global/data child idea is currently present under
`ideas/open/` beyond the umbrella classification note in idea `354`.

## Suggested Next

Next packet should be supervisor/plan-owner routing: either activate or create
a separate global aggregate/data materialization idea for the `main` global
`gs` lane loads, or decide whether this runbook is exhausted and should move to
Step 5 guard/closure review with the out-of-scope boundary documented.

## Watchouts

- `build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log` is stale from
  an older byval-home failure; the fresh delegated proof result is in
  `test_after.log`.
- Do not broaden this plan into global/data work. The active idea's non-goals
  explicitly exclude unrelated global/data-section materialization.
- The prepared dump is sufficient to classify the first shape: function
  `main`, block `entry`, inst `0`, `load_local i32 ... addr gs`; base kind is
  not published as prepared `frame_slot` or `pointer_value` for that load.

## Proof

Ran the delegated proof command exactly:

```sh
{ cmake --build --preset default && { mkdir -p "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c"; "$PWD/build/c4cll" --dump-prepared-bir -I "$PWD/tests/c/external/gcc_torture" --target riscv64-linux-gnu "$PWD/tests/c/external/gcc_torture/src/20030914-2.c" > "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log"; dump_rc=$?; cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/20030914-2.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/clang.bin" -DOUT_OBJECT="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.o" -DOUT_C4C_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.bin" -DCASE_TIMEOUT_SEC=20 -P "$PWD/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake"; case_rc=$?; exit $((dump_rc || case_rc)); }; } > test_after.log 2>&1
```

Result: failed after a successful no-op build. The prepared dump was produced
at `build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log`, and the
object-route case failed at the local-memory diagnostic above. Proof log:
`test_after.log`.
