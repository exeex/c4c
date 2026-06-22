Status: Active
Source Idea Path: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared Pointer-Local Inputs

# Current Packet

## Just Finished

Completed plan.md Step 1 inspection for the two RV64 local-pointer candidate
cases.

Observed prepared facts:

- `defined_global_array_pointer.c` lowers to `store_local %lv.p, ptr %t3`,
  then `load_local ptr %lv.p -> %t4`, then `load_local i32 %t7.addr,
  addr %t4+12`.
- `defined_global_array_pointer_store.c` lowers to `store_local %lv.p,
  ptr %t3`, then `load_local ptr %lv.p -> %t4`, then `store_local
  %t6.store.addr, i32 9, addr %t4+8`, then direct `load_global i32 @arr,
  offset 8`.
- The local pointer home is prepared in both cases as stack object `#0`
  named `%lv.p`, `source_kind=local_slot`, `type=ptr`, `size=8`, `align=8`,
  with frame slot `#0` at offset `0`.
- The local pointer store and reload have prepared addressing records:
  `inst_index=0 base=frame_slot stored=%t3 frame_slot=#0 offset=0 size=8
  align=8` and `inst_index=1 base=frame_slot result=%t4 frame_slot=#0
  offset=0 size=8 align=8`.
- The later pointer-value i32 load case has `inst_index=2
  base=pointer_value result=%t7 pointer=%t4 offset=12 size=4 align=4`.
- The later pointer-value i32 store case has `inst_index=2
  base=pointer_value pointer=%t4 offset=8 size=4 align=4`.
- The store case also preserves the accepted idea 304 direct global-symbol
  path for the reload: `inst_index=3 base=global_symbol result=%t11
  symbol=arr offset=8 size=4 align=4`.

Missing prepared/owner fact:

- `%t3` is the value stored into the local pointer slot for `&arr[0][0]`, but
  the prepared dump does not publish a direct global-symbol address
  materialization for `%t3`. Existing RV64 local pointer-store emission only
  consumes frame-slot address materializations, so asm generation currently
  stops after the prologue before emitting the first `store_local`.

## Suggested Next

Implement Step 2 in the RV64 prepared global/local memory owner surface: add a
semantic direct global array address materialization helper for defined `i32`
globals plus constant byte offsets, then let the pointer `store_local` path
store that materialized address into the prepared `%lv.p` frame slot.

## Watchouts

- Do not broaden into pointer-valued global initializers, global pointer data
  slots, GOT/object semantics, dynamic global indexing, or testcase-specific
  matching.
- Do not register runtime cases until the real RV64 asm route executes them
  under qemu with expected exit codes.
- Preserve fail-closed behavior for unsupported pointer/global forms instead
  of accepting incomplete assembly that later segfaults under qemu.
- The pointer-value load/store metadata is already present for `%t4` with
  constant offsets `12` and `8`; the immediate blocker is materializing `%t3`
  as a real global address and preserving it through the existing pointer local
  slot.

## Proof

Inspection-only packet. Ran:

- `./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer.c`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer.c`
- `./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer_store.c`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer_store.c`
- `./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer.c -o /tmp/defined_global_array_pointer.s`
- `./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer_store.c -o /tmp/defined_global_array_pointer_store.s`

No build/test proof or `test_after.log` was required by this packet.
