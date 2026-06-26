Status: Active
Source Idea Path: ideas/open/383_rv64_global_aggregate_lane_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Rerun Representative And Route Next Boundary

# Current Packet

## Just Finished

Step 4 reran the `src/20030914-2.c` representative after the Step 3 diagnostic
routing change.

The representative now fails at the precise missing prepared global-symbol
memory-access publication boundary:

```text
unsupported_global_data: RV64 object route requires prepared global-symbol memory access facts for LoadLocalInst global-address lanes
```

The first concrete shape remains `main`, block `entry`, inst `0`,
`LoadLocalInst`:

```text
%t0.global.aggregate.load.0 = bir.load_local i32 %t0.0, addr gs
```

The paired destination frame-slot stores still have prepared local-memory facts;
the source global aggregate lanes do not have authoritative prepared
`GlobalSymbol` memory-access facts. This is an upstream prepared-data
publication owner, not an RV64 object-emission lowering owner for raw
`LoadLocalInst` address spelling.

## Suggested Next

Supervisor should route this to closure/guard review for the current runbook or
to a separate upstream prepared-data publication idea. This idea has established
that RV64 object emission can consume explicit prepared `GlobalSymbol` access
facts and should fail closed when the representative only publishes raw
`LoadLocalInst` global-address lanes.

## Watchouts

- The representative did not advance beyond the first global aggregate source
  lane, so there is no newly exposed aggregate `va_arg`, terminator, call, or
  control-flow owner from this proof.
- Do not implement target-side inference from raw `addr gs` spelling to make
  this case pass; that would bypass the prepared-data contract established by
  this runbook.
- Proof artifacts: `test_after.log` and
  `build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log`.

## Proof

Delegated Step 4 proof:

```sh
{ cmake --build --preset default && { mkdir -p "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c"; "$PWD/build/c4cll" --dump-prepared-bir -I "$PWD/tests/c/external/gcc_torture" --target riscv64-linux-gnu "$PWD/tests/c/external/gcc_torture/src/20030914-2.c" > "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log"; dump_rc=$?; cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/20030914-2.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/clang.bin" -DOUT_OBJECT="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.o" -DOUT_C4C_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.bin" -DCASE_TIMEOUT_SEC=20 -P "$PWD/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake"; case_rc=$?; exit $((dump_rc || case_rc)); }; } > test_after.log 2>&1
```

Result: failed after a successful no-op build at the expected representative
boundary with `unsupported_global_data`. The supervisor-selected proof is
sufficient for this documentation packet because it confirms the post-Step 3
diagnostic and owner classification. Proof log: `test_after.log`; prepared
dump: `build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log`.
