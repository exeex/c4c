Status: Active
Source Idea Path: ideas/open/339_aarch64_scalar_local_storage_writeback_sizing.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar Local Slot Sizes And Writeback Path

# Current Packet

## Just Finished

Step 1 localized the first bad boundary for focused idea 339.

The semantic BIR instruction stream is still correct for both target cases:
`00086` stores `i16 0` to `%lv.x`, reloads it, truncates the incremented value
back to `i16`, stores `%t4` to `%lv.x`, then reloads `%lv.x` for the compare;
`00111` stores `i16 1` to `%lv.s`, stores `i64 1` to `%lv.l`, loads both,
truncates the subtraction back to `i16`, stores `%t6` to `%lv.s`, then reloads
`%lv.s` for the return.

The first bad concrete facts appear in prepared stack layout/addressing:

- `00086`: `%lv.x` is `type=i16` but object `#0` and slot `#0` both have
  `size=0`, `frame_size=0`, and all four prepared local accesses report
  `size=0`. The final AArch64 output then emits no store for either
  initialization or writeback and reloads from `[sp]`.
- `00111`: `%lv.l` has a real `size=8` slot/accesses, but `%lv.s` remains
  `type=i16 size=0`, is assigned at offset `8` outside the 8-byte local frame,
  and the three short accesses report `size=0`. The final AArch64 output stores
  and reloads `l`, but loads stale `%lv.s` from `[sp, #8]`, emits no short
  initialization/writeback store, and later collides with the saved `x20`
  slot at the same offset.

Likely implementation surface: `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
currently creates scalar local slots with no `size_bytes`, relying on prepared
normalization; `src/backend/prealloc/stack_layout/stack_layout.hpp` and the
local access builders in
`src/backend/prealloc/stack_layout/coordinator.cpp` then normalize through a
fallback size table that lacks `I16`, so `i16` slots/accesses stay zero-sized.
`src/backend/prealloc/legalize.cpp` has the same `I16` omission in its local
slot size helper, so it does not repair preexisting zero-sized `I16` slot facts
during legalization.

## Suggested Next

Execute Step 2 from `plan.md`: repair the general scalar local sizing path for
`I16` non-address-exposed locals, then prove prepared stack layout and prepared
addressing publish nonzero size/alignment for short `store_local`/`load_local`
before relying on final assembly.

## Watchouts

- Keep this owner focused on scalar local storage/writeback sizing for
  non-address-exposed scalar locals.
- Do not fold in parked FP comparison/expression, pointer/null conditional,
  broad return-spill, aggregate, variadic, timeout, runner, expectation, or
  CTest-registration work.
- Do not special-case `00086`, `00111`, stack offsets, variable names, or
  emitted instruction strings.
- The AArch64 local load/store consumer is downstream of the first bad fact:
  zero-sized prepared accesses cannot use direct frame-slot addressing and
  zero-sized frame slots collapse frame size. Fixing only AArch64 emission would
  be route drift unless prepared slot/access facts are fixed first.
- Focused Step 2 coverage should assert a general backend contract such as
  prepared `i16` local object/slot/access `size=2 align=2`, short
  initialization store, short writeback store after truncation, and reload
  using the same slot. Add final-assembly checks only after that contract is
  pinned.

## Proof

Non-mutating localization probes only; no implementation or tests changed.
`test_after.log` was not touched because this packet explicitly marked it out
of bounds.

Commands run:

- `./build/c4cll --dump-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00086.c > /tmp/c4c_00086.semantic.txt`
- `./build/c4cll --dump-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00111.c > /tmp/c4c_00111.semantic.txt`
- `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00086.c > /tmp/c4c_00086.prepared.txt`
- `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00111.c > /tmp/c4c_00111.prepared.txt`
- `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00086.c -o /tmp/c4c_00086.s`
- `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00111.c -o /tmp/c4c_00111.s`
