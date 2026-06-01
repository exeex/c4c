Status: Active
Source Idea Path: ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Compare Predicate Helper Logic

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by folding the duplicated compare
predicate-to-condition-code mapping in `machine_printer.cpp` back to the
existing AArch64-local comparison helper.

Before inventory:

- `machine_printer.cpp` had a local unnamed-namespace
  `compare_branch_condition` duplicate mapping compare predicates to AArch64
  branch condition suffixes for fused compare branch printing.
- `comparison.cpp` already owned the same AArch64-local mapping through
  `branch_condition_suffix`, exported by `comparison.hpp` and used by compare
  lowering.

After inventory:

- Removed the local `compare_branch_condition` duplicate from
  `machine_printer.cpp`.
- Fused compare branch printing now calls `branch_condition_suffix` directly
  through the existing `comparison.hpp` include.
- Condition-code spelling, unsupported handling, compare/branch formatting,
  comparison lowering, shared BIR/prealloc/ABI files, and tests are unchanged.

## Suggested Next

Continue with the next supervisor-selected fold-back family. A likely remaining
candidate is Step 4 local scalar record assembly helper cleanup if the Step 2
and Step 3 helper families are considered complete.

## Watchouts

- `integer_type_bit_width` in `machine_printer.cpp` is similar but not identical
  to `integer_scalar_bit_width` because it includes `I1 -> 1`; do not fold that
  into Step 2 unless the call sites are checked explicitly.
- `scalar_storage_register_view` still has same-shaped local duplicates outside
  this packet in non-owned files such as globals/atomics/cast/memory; leave
  those for a supervisor-selected packet if they become in scope.
- `swapped_compare_predicate` remains local printer logic for operand swapping;
  it is not the same mapping as condition-code spelling and was intentionally
  left in place.
- Keep BIR facts and `abi::convert_prepared_register` as inputs rather than
  re-deriving shared semantics, and do not move AArch64 record schemas, opcode
  decisions, register-name spelling, scratch selection, or printer formatting
  into shared helper code.

## Proof

Ran the delegated proof command:

`cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_branch_compare_records|backend_aarch64_compare_branch_candidate_records|backend_aarch64_branch_compare_contract|backend_aarch64_prepared_branch_records|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' >> test_after.log`

Result: passed. `test_after.log` contains the successful build and 7/7 passing
selected tests.
