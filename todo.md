Status: Active
Source Idea Path: ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Scalar Type And Register-View Helpers

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by folding the duplicated scalar storage
type-to-register-view helper in `instruction.cpp` back to the exported
AArch64-local ALU helper surface.

Before inventory:

- `instruction.cpp` had an unnamed-namespace `scalar_storage_register_view`
  duplicate that first called `scalar_register_view` for integer and pointer
  storage, then fell back to `scalar_fp_register_view` for floating scalar
  storage.
- `alu.cpp` had the same `scalar_storage_register_view` body in its local ALU
  helper set, with live ALU and prepared scalar call sites already using it.

After inventory:

- `alu.hpp` now exposes `scalar_storage_register_view` as the minimal shared
  AArch64-local ALU helper needed by `instruction.cpp`.
- `alu.cpp` keeps the unchanged helper body as the single implementation, next
  to the exported scalar register-view helper surface.
- Removed the local `instruction.cpp` storage-view helper and its now-unused
  `float_ops.hpp` include; instruction prepared-register code now uses the
  shared ALU helper through `alu.hpp`.
- Left `machine_printer.cpp`, record construction, opcode decisions, shared
  BIR/prealloc/ABI files, tests, and unrelated storage-view duplicates
  unchanged.

## Suggested Next

Continue Step 2 with any remaining scalar type/register-view helper fold-back
inside the active owned surface, or move to Step 3 compare predicate folding if
the supervisor considers the Step 2 owned helper families complete.

## Watchouts

- `integer_type_bit_width` in `machine_printer.cpp` is similar but not identical
  to `integer_scalar_bit_width` because it includes `I1 -> 1`; do not fold that
  into Step 2 unless the call sites are checked explicitly.
- The compare predicate condition-code mapping remains duplicated by design for
  this slice; folding it back should be a separate packet so type/view helper
  movement does not mix with branch/printer condition spelling.
- `scalar_storage_register_view` still has same-shaped local duplicates outside
  this packet in non-owned files such as globals/atomics/cast/memory; leave
  those for a supervisor-selected packet if they become in scope.
- Keep BIR facts and `abi::convert_prepared_register` as inputs rather than
  re-deriving shared semantics, and do not move AArch64 record schemas, opcode
  decisions, register-name spelling, scratch selection, or printer formatting
  into shared helper code.

## Proof

Ran the delegated proof command:

`cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_record_contract|backend_aarch64_target_record_core_contract|backend_aarch64_instruction_dispatch)$' >> test_after.log`

Result: passed. `test_after.log` contains the successful build and 6/6 passing
selected tests.
