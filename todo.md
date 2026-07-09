Status: Active
Source Idea Path: ideas/open/626_prepared_dynamic_frame_callee_saved_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Prepared Save-Slot Placement Facts

# Current Packet

## Just Finished

Step 3 corrected the prepared save-slot placement producer so idea 626's
dynamic-stack extension is GPR-only while fixed-frame behavior remains
unchanged, then proved the producer on representative idea-626 rows before RV64
object emission.

Changed files:

- `src/backend/prealloc/frame_plan.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `todo.md`
- `test_after.log`

Producer behavior:

- `populate_frame_plan(PreparedBirModule&)` now uses
  `should_publish_saved_register_slot_placement(...)` to preserve the original
  fixed-frame all-bank saved-register slot-placement path while limiting
  dynamic-stack publication to `PreparedRegisterBank::Gpr`.
- The placement remains producer-authored from existing frame-layout and
  register-placement authority: `next_prepared_frame_slot_id(...)`,
  `plan.frame_size_bytes`, `align_prepared_offset(...)`,
  `saved_register_slot_unit_size(...)`, and
  `make_saved_register_slot_placement(...)`.
- The fail-closed checks are unchanged: rows still skip placement publication
  when the bank has no slot unit size or the saved register has no target
  `placement`.
- Dynamic FPR/vector saved-register rows intentionally remain without
  `slot_placement` in this idea; that boundary avoids drifting into FPR/vector
  dynamic-frame placement work.
- Generated saved-register save slots remain placement facts on
  `PreparedSavedRegister::slot_placement`; they are not added to
  `frame_slot_order`.

Test coverage:

- Added a RISC-V dynamic-stack fixture with a fixed local slot, stack
  save/dynamic alloca/restore calls, a live GPR callee-saved value across a
  call, and non-GPR saved-register boundary coverage.
- Added `check_dynamic_stack_callee_saved_slot_placement_contract()` to prove
  the dynamic frame row has `has_dynamic_stack=yes`, uses the frame pointer for
  fixed slots, preserves a GPR through callee-saved authority, publishes a
  complete `PreparedSavedRegisterSlotPlacement`, places the generated save slot
  after fixed frame slots, and exposes `slot_placement=slot#...+stack...`
  facts in `prepare::print(prepared)` before object emission.
- The same focused test requires a dynamic non-GPR saved-register row to remain
  without `slot_placement` and checks that the prepared dump exposes only the
  GPR save-slot placement facts.
- No CMake changes were required because the coverage lives in the existing
  backend prepared frame/stack/call contract test.

Representative row evidence:

- Ran `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/20040811-1.c`; the prepared dump for
  `@main` reports `frame_size=24 frame_alignment=8 has_dynamic_stack=yes
  fixed_slots_use_fp=yes` and complete saved GPR placement facts:
  `s1 slot_placement=slot#4+stack24 slot_size=8 slot_align=8` and
  `s2 slot_placement=slot#5+stack32 slot_size=8 slot_align=8`.
- Also spot-checked the other named representatives with the same command:
  `pr43220.c` reports `s1 slot#7+stack40` and `s2 slot#8+stack48`, and
  `vla-dealloc-1.c` reports `s1 slot#4+stack24` and `s2 slot#5+stack32`.
- The representative evidence is from the prepared dump path and occurs before
  RV64 object emission; no implementation changes were needed during this
  evidence packet.

## Suggested Next

Run Step 4 as a narrow RV64 consumer-guard packet: make RV64 object-route
admission consume only complete producer-published
`PreparedSavedRegisterSlotPlacement` facts for dynamic-stack callee-saved GPR
rows, and fail closed when the placement is absent or incomplete. Keep the
consumer from deriving save-slot offsets from frame size, register order,
source filename, final assembly shape, or dynamic-frame heuristics.

## Watchouts

- Step 3 intentionally did not touch RV64 object consumer implementation,
  unsupported markers, allowlists, or unrelated expectations.
- Consumer work must read the producer-published `slot_placement` carrier and
  reject incomplete rows instead of recovering missing placement downstream.
- Dynamic FPR/vector placement remains out of scope for idea 626 and should not
  be inferred from this GPR-only producer path.
- Dynamic-frame object lowering, move-bundle authority, local/global memory
  repair, and call policy remain outside this slice.
- Saved-register save slots are still not ordinary fixed frame slots and should
  not be treated as entries in `frame_slot_order`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed. Proof log: `test_after.log`.
