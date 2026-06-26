Status: Active
Source Idea Path: ideas/open/382_rv64_object_route_prepared_local_memory_addressing.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Supported And Rejected Fixtures

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit for `src/20030914-2.c`.

Diagnostic site: `src/backend/mir/riscv/codegen/object_emission.cpp`
`diagnose_unsupported_prepared_instruction_fragment`, in the
`local_memory_diagnostic` preflight for `LoadLocalInst` and `StoreLocalInst`.
The proof run reports:
`unsupported_local_memory_access: RV64 object route requires prepared frame-slot
or pointer-value base-plus-offset local memory addressing`.

Prepared facts at the representative boundary, from
`build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log`:

- Function `@f` has a byval aggregate parameter `%p.pa` with BIR type `ptr
  byval(size=72, align=4)`.
- Stack layout publishes object `#18` for `%p.pa` with
  `source_kind=byval_param`, `size=72`, `align=4`, `address_exposed=yes`,
  `requires_home_slot=yes`, and `permanent_home_slot=yes`.
- Frame slot `#0` covers that object at offset `0`, size `72`, align `4`, with
  a fixed location; value locations publish `%p.pa` as a stack-slot/frame-slot
  home at slot `#0`, stack offset `0`.
- The first local-memory access is `@f entry inst_index=0`:
  `base=pointer_value`, `result=%lv.param.p.pa.aggregate.param.copy.0`,
  `pointer=%p.pa`, `offset=0`, `size=4`, `align=4`,
  `base_plus_offset=yes`, `range_verdict=proven_in_bounds`,
  `address_space=default` by absence of a non-default marker in the prepared
  addressing dump.
- The same semantic access repeats for offsets `4, 8, ..., 68`, all 4-byte,
  4-aligned, base-plus-offset, proven-in-bounds loads from `%p.pa`.
- Each loaded lane is then stored to a distinct prepared frame slot:
  frame slots `#1..#18`, offsets `72..140`, size `4`, align `4`,
  `base=frame_slot`, `offset=0`, `size=4`, `align=4`,
  `base_plus_offset=yes`, `range_verdict=proven_in_bounds`.

First supportable semantic shape: a default-address-space, non-volatile,
prepared pointer-value local memory load where the pointer value is a byval
aggregate parameter with a fixed prepared stack-slot/frame-slot home, the
access is constant base-plus-offset, size `4`, alignment `4`, offset is
non-negative, signed-12-bit encodable, and proven within the 72-byte aggregate
home. This shape should be lowerable using only prepared metadata as a load
from the prepared byval home slot plus the prepared lane offset; the adjacent
frame-slot stores are already the simple 4-byte frame-slot local-memory shape.

Adjacent shapes to keep rejected: missing pointer-value identity, missing or
ambiguous stack/frame-slot home, non-`byval_param` aggregate home, non-default
address space, volatile access, dynamic or non-base-plus-offset addressing,
negative/out-of-range/unencodable offsets, unsupported access sizes outside
1/2/4/8 bytes, over-aligned accesses not justified by prepared facts, and
aggregate homes whose object/slot/home provenance does not agree.

## Suggested Next

Step 2 should add focused RV64 object-emission fixtures for the supported
byval-stack-slot pointer-value 4-byte lane load shape, including offset `0` and
the final in-bounds offset `68`, plus fail-closed fixtures for missing
identity/home, non-default address space, dynamic addressing, and out-of-bounds
or over-aligned variants.

## Watchouts

- `build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log` still contains
  the older `unsupported_byval_param_home` text; `test_after.log` and the fresh
  `prepared.log` are the authoritative artifacts from this packet.
- Existing focused tests already include `make_prepared_byval_stack_slot_param_module`
  and a rejection helper for byval stack-slot pointer accesses; Step 2 can
  extend that family instead of creating testcase-shaped coverage.
- Do not widen this into global aggregate lane loads from `gs` in `@main`; this
  packet only identifies the first local-memory shape at the current boundary.

## Proof

Ran the delegated proof command exactly:

```sh
cmake --build --preset default && { mkdir -p "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c"; "$PWD/build/c4cll" --dump-prepared-bir -I "$PWD/tests/c/external/gcc_torture" --target riscv64-linux-gnu "$PWD/tests/c/external/gcc_torture/src/20030914-2.c" > "$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log"; dump_rc=$?; cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/20030914-2.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/clang.bin" -DOUT_OBJECT="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.o" -DOUT_C4C_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_20030914-2.c/c4c.bin" -DCASE_TIMEOUT_SEC=20 -P "$PWD/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake"; case_rc=$?; exit $((dump_rc || case_rc)); } > test_after.log 2>&1
```

Result: build completed with `ninja: no work to do`; prepared dump was written;
the case compile failed at the expected current object-route boundary. Proof
log: `test_after.log`. Generated audit artifact:
`build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log`. Supervisor
comparison confirmed `test_before.log` and `test_after.log` are identical and
both reproduce the same `unsupported_local_memory_access` diagnostic.
