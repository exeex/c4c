Status: Active
Source Idea Path: ideas/open/384_prepared_global_symbol_memory_access_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Producer Source For Global Lanes

# Current Packet

## Just Finished

Step 1 audited the producer path for the first `src/20030914-2.c` global
aggregate lane. The exact producer surface is
`BirFunctionLowerer::lower_memory_load_inst` in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp`, in the aggregate-load path
for `load.ptr.kind() == LirOperandKind::Global`, which emits
`main` / `entry` / inst `0` as:

```text
%t0.global.aggregate.load.0 = bir.load_local i32 %t0.0, addr gs
```

That producer already has the supportable lane facts: symbol display identity
`gs`, static byte offset `0` for the first lane and `4..68` for later lanes,
lane size `4`, default address space and volatility through the default
`bir::MemoryAddress`, aggregate storage size `72`, linear global addressing
authority from `GlobalInfo::supports_linear_addressing`, and per-lane storage
relation through the declared local aggregate leaf slots. The first producer
gap is semantic global identity: it fills `MemoryAddress::base_name` but not
`MemoryAddress::base_link_name_id`, even though `global_it->second.link_name_id`
is available from `global_types_`.

Prepared stack-layout already has the downstream publication surface:
`build_direct_symbol_backed_access(const bir::LoadLocalInst&)` calls
`build_direct_symbol_backed_address`, which can publish
`PreparedAddressBaseKind::GlobalSymbol` with symbol name, byte offset, size,
alignment, default address-space/volatile facts, base-plus-offset support,
materialization policy, provenance, and object-size range authority. It rejects
this first lane today because `resolve_prepared_global_symbol_address` accepts
structured `LinkNameId` identity, while the emitted BIR lane only carries raw
spelling; raw link-name spelling compatibility is intentionally disabled for
normal `LoadLocalInst` global lanes.

Conclusion: the first supportable shape can be published directly after the
producer stamps `base_link_name_id = global_it->second.link_name_id` on these
`GlobalSymbol` lane addresses. No narrower upstream contract prerequisite was
found beyond carrying the already-available semantic global id; target-side
RV64 must continue consuming the explicit prepared `GlobalSymbol` access and
must not infer it from raw `addr gs` text.

## Suggested Next

Delegate Step 2 to add focused prepared-contract coverage for a
`LoadLocalInst` global aggregate lane whose `bir::MemoryAddress` carries both
`base_kind = GlobalSymbol` and a valid `base_link_name_id`, proving prepared
publication of symbol name, offset, size, alignment, address-space/volatile,
base-plus-offset, materialization policy, provenance, and object range facts.

## Watchouts

- Do not add target-side fallback inference from raw `LoadLocalInst addr
  <global>` spelling; idea 383 closed that route as fail-closed.
- Keep behavior independent of testcase `src/20030914-2.c`, global `gs`, and
  lane offset `68`.
- The producer fix should use `global_it->second.link_name_id`, not a raw-name
  lookup in prepared stack layout. The existing raw-spelling compatibility
  path is intentionally narrow and should not become the normal publication
  route for semantic globals.
- Keep `plan.md` and this `todo.md` pointed at
  `ideas/open/384_prepared_global_symbol_memory_access_publication.md`.

## Proof

Delegated proof ran exactly:

```sh
{ cmake --build --preset default && { mkdir -p "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c"; "$PWD/build/c4cll" --dump-prepared-bir -I "$PWD/tests/c/external/gcc_torture" --target riscv64-linux-gnu "$PWD/tests/c/external/gcc_torture/src/20030914-2.c" > "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log"; dump_rc=$?; cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/20030914-2.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/clang.bin" -DOUT_OBJECT="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.o" -DOUT_C4C_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.bin" -DCASE_TIMEOUT_SEC=20 -P "$PWD/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake"; case_rc=$?; exit $((dump_rc || case_rc)); }; } > test_after.log 2>&1
```

Result: failed after successful no-op build at the expected current boundary:

```text
unsupported_global_data: RV64 object route requires prepared global-symbol memory access facts for LoadLocalInst global-address lanes
```

Proof log: `test_after.log`. Prepared dump:
`build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log`.
