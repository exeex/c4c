Status: Active
Source Idea Path: ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Scalar Type And Register-View Helpers

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by folding the duplicated floating scalar
type-to-FP/SIMD-register-view helpers in `instruction.cpp` and
`machine_printer.cpp` back to the exported AArch64-local
`scalar_fp_register_view` helper from `float_ops.hpp`.

Before inventory:

- `instruction.cpp` had an unnamed-namespace `scalar_fp_register_view`, mapping
  `F32` to `S`, `F64` to `D`, and rejecting integer, pointer, void, and `F128`
  types.
- `machine_printer.cpp` had `floating_register_view` with the same floating
  scalar mapping, though it no longer had live call sites in the current tree.
- `float_ops.cpp` already exported `scalar_fp_register_view` with the same
  mapping through `float_ops.hpp`.

After inventory:

- Removed the local `instruction.cpp` FP view helper; scalar storage view
  fallback now calls the exported `scalar_fp_register_view`.
- Removed the unused local `machine_printer.cpp` floating view helper.
- Left record construction, printer formatting, opcode decisions, shared
  BIR/prealloc/ABI files, tests, and unrelated helper families unchanged.

## Suggested Next

Continue Step 2 with the scalar storage register-view fold-back family as a
separate packet, after checking whether the remaining duplicates in
`instruction.cpp`, `alu.cpp`, `globals.cpp`, `atomics.cpp`, and `cast_ops.cpp`
can share one AArch64-local helper without widening beyond the supervisor-owned
file set.

## Watchouts

- `integer_type_bit_width` in `machine_printer.cpp` is similar but not identical
  to `integer_scalar_bit_width` because it includes `I1 -> 1`; do not fold that
  into Step 2 unless the call sites are checked explicitly.
- The compare predicate condition-code mapping remains duplicated by design for
  this slice; folding it back should be a separate packet so type/view helper
  movement does not mix with branch/printer condition spelling.
- `scalar_storage_register_view` still has same-shaped local duplicates outside
  this packet; fold it only if the helper boundary stays AArch64-local and does
  not move record schemas, storage-plan policy, or printer spelling.
- Keep BIR facts and `abi::convert_prepared_register` as inputs rather than
  re-deriving shared semantics, and do not move AArch64 record schemas, opcode
  decisions, register-name spelling, scratch selection, or printer formatting
  into shared helper code.

## Proof

Ran the delegated proof command:

`cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_record_contract|backend_aarch64_target_record_core_contract|backend_aarch64_instruction_dispatch)$' >> test_after.log`

Result: passed. `test_after.log` contains the successful build and 7/7 passing
selected tests.
