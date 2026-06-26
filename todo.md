Status: Active
Source Idea Path: ideas/open/384_prepared_global_symbol_memory_access_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Rerun Representative And Route Next Boundary

# Current Packet

## Just Finished

Step 4 reran the representative `src/20030914-2.c` RV64 torture object proof
after Step 3. The previous global-symbol publication boundary advanced:
`main`, `entry`, inst `0` is now
`%t0.global.aggregate.load.0 = bir.load_global i32 @gs`, with repeated
`bir.load_global i32 @gs, offset N` lanes through offset `68`.

The prepared memory-access table now publishes those lanes as
`base=global_symbol` accesses, for example `block=entry inst_index=0
base=global_symbol result=%t0.global.aggregate.load.0 symbol=gs offset=0
size=4 align=4 base_plus_offset=yes`, and the same form through
`inst_index=34` / offset `68`.

The new first representative boundary is no longer global data publication.
The object route reaches `main`, `entry`, inst `36`:

```text
%t1 = bir.call i32 f(ptr byval(size=72, align=4) %t0, i32 4660)
```

The prepared callsite record classifies that as `wrapper=same_module`,
`callee=f`, `arg0 bank=aggregate_address from=register:s1 to=none`, and
`arg1 bank=gpr from=immediate:4660 to=a1`. The proof fails there with the
generic object-route diagnostic:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

## Suggested Next

Route the remaining representative boundary to the next object-route owner for
same-module call lowering with a byval aggregate/address argument. This is
outside the current prepared global-symbol publication idea; the supervisor
should decide whether to reactivate an existing byval aggregate/call owner from
the umbrella notes or ask the plan owner to create a narrower child.

## Watchouts

- Raw/no-id `LoadLocalInst` global lanes intentionally remain unpublished and
  fail-closed.
- RV64 object emission is now consuming explicit `LoadGlobalInst` prepared facts
  for this representative; do not reintroduce target-side inference from raw
  global-address spelling.
- The current failing instruction is a call instruction, not a terminator,
  aggregate `va_arg`, non-register entry parameter home, or residual global data
  publication gap.
- Keep `plan.md` and this `todo.md` pointed at
  `ideas/open/384_prepared_global_symbol_memory_access_publication.md`.

## Proof

Delegated proof ran exactly:

```sh
{ cmake --build --preset default && { mkdir -p "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c"; "$PWD/build/c4cll" --dump-prepared-bir -I "$PWD/tests/c/external/gcc_torture" --target riscv64-linux-gnu "$PWD/tests/c/external/gcc_torture/src/20030914-2.c" > "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log"; dump_rc=$?; cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/20030914-2.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/clang.bin" -DOUT_OBJECT="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.o" -DOUT_C4C_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.bin" -DCASE_TIMEOUT_SEC=20 -P "$PWD/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake"; case_rc=$?; exit $((dump_rc || case_rc)); }; } > test_after.log 2>&1
```

Result: failed after a successful no-op build at the expected next
classification boundary:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

Proof log: `test_after.log`. Prepared dump:
`build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log`.
