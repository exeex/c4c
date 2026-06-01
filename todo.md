Status: Active
Source Idea Path: ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Scalar Type And Register-View Helpers

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by folding the duplicated integer scalar
type-to-GP-register-view helpers in `comparison.cpp` and `machine_printer.cpp`
back to the exported AArch64-local `scalar_register_view` helper from `alu.hpp`.

Before inventory:

- `comparison.cpp` had `comparison_scalar_register_view`, mapping
  `I1/I8/I16/I32` to `W`, `I64/Ptr` to `X`, and rejecting other types.
- `machine_printer.cpp` had `compare_register_view` with the same integer and
  pointer scalar mapping.
- `alu.cpp` already exported `scalar_register_view` with the same mapping.

After inventory:

- Removed `comparison_scalar_register_view`; condition-register and fused
  compare operand setup now call `scalar_register_view` directly.
- Removed `compare_register_view`; compare operand spelling and compare branch
  setup now call `scalar_register_view` directly.
- Left printer formatting, condition-code spelling, record schemas, opcode
  decisions, shared BIR/prealloc/ABI files, tests, and unrelated helper
  families unchanged.

## Suggested Next

Continue with the next fold-back family as a separate packet, likely the
floating scalar register-view or scalar storage register-view duplication,
depending on supervisor route choice.

## Watchouts

- `integer_type_bit_width` in `machine_printer.cpp` is similar but not identical
  to `integer_scalar_bit_width` because it includes `I1 -> 1`; do not fold that
  into Step 2 unless the call sites are checked explicitly.
- The compare predicate condition-code mapping remains duplicated by design for
  this slice; folding it back should be a separate packet so type/view helper
  movement does not mix with branch/printer condition spelling.
- `scalar_storage_register_view` depends on the FP scalar view helper; leave it
  in place until the shared integer and FP view helpers are stable.
- Keep BIR facts and `abi::convert_prepared_register` as inputs rather than
  re-deriving shared semantics, and do not move AArch64 record schemas, opcode
  decisions, register-name spelling, scratch selection, or printer formatting
  into shared helper code.

## Proof

Ran the delegated proof command:

`cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|backend_aarch64_scalar_alu_records|backend_aarch64_branch_compare_records|backend_aarch64_branch_compare_contract)$' >> test_after.log`

Result: passed. `test_after.log` contains the successful build and 5/5 passing
selected tests.
