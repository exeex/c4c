Status: Active
Source Idea Path: ideas/open/383_rv64_global_aggregate_lane_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Global Aggregate Lane Facts

# Current Packet

## Just Finished

Step 1 audited the prepared facts for the first `src/20030914-2.c`
representative global aggregate lane.

Exact first failing shape from prepared artifacts:

```text
function=main block=entry inst=0
%t0.global.aggregate.load.0 = bir.load_local i32 %t0.0, addr gs
```

Facts published in `build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log`:

- Source operation identity: `LoadLocalInst`, result
  `%t0.global.aggregate.load.0`, type `i32`, destination local name `%t0.0`,
  address text `addr gs`.
- Lane facts visible in the instruction text: first lane spelling `addr gs`.
  Repeated sibling load spellings appear as `addr gs+4`, `addr gs+8`, through
  `addr gs+68` with the same `i32` load result type. No source-lane access
  size, alignment, range, section, initializer payload, or relocation fact is
  published for inst `0` as a prepared memory-access record.
- Value storage for the loaded lane is published as register storage:
  value_id `20`, `encoding=register`, bank `gpr`, placement
  `gpr:caller_saved#0/w1`, reg `t0`, width `1`.
- The paired destination store at inst `1` has a prepared frame-slot access:
  `base=frame_slot`, `frame_slot=#19`, `offset=0`, `size=4`, `align=4`,
  `base_plus_offset=yes`, `range_verdict=proven_in_bounds`.
- The first source load at inst `0` has no matching
  `prepared-addressing` access record. There is no prepared global-symbol
  memory access for `main`/`entry`/`0`, no published source symbol id beyond the
  instruction address spelling, no section or initializer/lane payload, no
  global-object size/alignment facts, and no relocation/address-use record for
  this source load.
- The only `main` address materialization published near this sequence is the
  later frame-slot address materialization for call argument `%t0` at inst
  `36`: `kind=frame_slot`, `offset=0`, `address_space=default`,
  `tls_global=no`, `tls_address_space=no`.

Classification: upstream prepared-data publication gap. RV64 object emission
already consumes idea 357-style prepared global data when the prepared module
publishes a `LoadGlobalInst`/`StoreGlobalInst` plus a `PreparedMemoryAccess`
whose base is `GlobalSymbol`, symbol name is present, address space is default,
size/align/offset are bounded, and the module global provides object storage.
The representative is instead a `LoadLocalInst` with no prepared global-symbol
access record, so target emission cannot consume it without inferring global
identity, section, initializer bytes, lane payload, or relocation semantics
from raw `addr gs` text. That inference is outside the closed idea 357
contract.

## Suggested Next

Add or route an upstream prepared-data publication packet that publishes
authoritative `GlobalSymbol` prepared memory-access facts for aggregate
source lanes represented today as `load_local ... addr <global>`, then rerun
the same representative to confirm RV64 object emission reaches the existing
prepared global-data path or a precise `unsupported_global_data` boundary.

## Watchouts

- Do not implement RV64 object-emission fallback parsing of `addr gs`; that
  would bypass the prepared contract and reopen idea 357 by inference.
- Keep the first focused test semantic: a prepared `LoadLocalInst` with an
  explicit global-symbol access record is target-consumable; the same raw
  `LoadLocalInst` without that record is an upstream publication gap.
- `src/20030914-2.c` and the spelling `gs` remain representative evidence
  only, not behavior keys.

## Proof

Delegated Step 1 proof:

```sh
{ cmake --build --preset default && { mkdir -p "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c"; "$PWD/build/c4cll" --dump-prepared-bir -I "$PWD/tests/c/external/gcc_torture" --target riscv64-linux-gnu "$PWD/tests/c/external/gcc_torture/src/20030914-2.c" > "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log"; dump_rc=$?; cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/20030914-2.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/clang.bin" -DOUT_OBJECT="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.o" -DOUT_C4C_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.bin" -DCASE_TIMEOUT_SEC=20 -P "$PWD/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake"; case_rc=$?; exit $((dump_rc || case_rc)); }; } > test_after.log 2>&1
```

Result: failed after a successful no-op build at the expected representative
boundary:

```text
unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing
```

The failing result is sufficient for this audit slice because the command
generated the prepared contract artifact and preserved the current object-route
diagnostic. Proof log: `test_after.log`. Prepared dump:
`build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log`.
