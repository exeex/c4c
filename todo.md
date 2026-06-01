Status: Active
Source Idea Path: ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Duplicate Scalar Helper Logic

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory of duplicate AArch64 scalar helper logic
across the owned inspection files.

Duplicate helper logic found:

- Integer scalar type -> GP register view mapping is repeated as
  `scalar_register_view` in `alu.cpp`, `comparison_scalar_register_view` in
  `comparison.cpp`, and `compare_register_view` in `machine_printer.cpp`; all
  map `I1/I8/I16/I32` to `W`, `I64/Ptr` to `X`, and reject non-integer or
  unsupported scalar types.
- Floating scalar type -> FP/SIMD register view mapping is repeated as
  `scalar_fp_register_view` in `instruction.cpp` and `floating_register_view`
  in `machine_printer.cpp`; both map `F32` to `S`, `F64` to `D`, and reject
  `F128` plus non-FP types.
- Scalar storage type -> register view composition is repeated as
  `scalar_storage_register_view` in `instruction.cpp` and `alu.cpp`; both first
  try the integer scalar view and then fall back to the FP scalar view.
- Integer scalar bit-width logic is repeated as `integer_scalar_bit_width` in
  `alu.cpp` and `integer_type_bit_width` in `machine_printer.cpp`; the printer
  variant includes `I1 -> 1`, while the ALU variant starts at `I8 -> 8`.
- Compare predicate -> AArch64 condition-code mapping is repeated as
  `branch_condition_suffix` in `comparison.cpp` and
  `compare_branch_condition` in `machine_printer.cpp`; both map
  `Eq/Ne/Slt/Sle/Sgt/Sge/Ult/Ule/Ugt/Uge` to
  `eq/ne/lt/le/gt/ge/lo/ls/hi/hs`.
- Register spelling with an explicit GP view is repeated as
  `scalar_gp_register_name_with_view` in `alu.cpp`,
  `emitted_register_name`'s viewed-register block in `comparison.cpp`, and
  `register_name_with_view` in `machine_printer.cpp`.
- Prepared named-value/home and storage lookup patterns are repeated between
  `alu.cpp` and `instruction.cpp`, but they are adjacent to record construction
  and are lower priority than the type/view and predicate helpers.

Target-local logic intentionally left in place for now:

- `instruction.hpp` record schemas, enum surfaces, status/error records, and
  opcode-family record ownership stay local; they define machine-record shape,
  not shared scalar semantics.
- `machine_opcode_from_scalar_*` in `instruction.cpp` and scalar ALU opcode
  selection in `alu.cpp` stay local; they select AArch64 machine records and
  opcodes rather than duplicate type/view helper facts.
- `machine_printer.cpp` operand spelling, immediate encodability, scratch
  choice, setup-line generation, and final formatting stay printer-local.
- `comparison.cpp` fused-compare branch fact discovery, prepared producer
  lookup, branch-target resolution, and emitted-condition lowering stay
  comparison-local.
- ALU publication, stack-slot, immediate acceptance, and emitted-register state
  mutation stay ALU-local unless a later packet proves a shared helper can
  preserve those ownership boundaries.

Selected first fold-back family for Step 2:

Fold back the integer scalar type -> GP register view helper first by creating
one shared AArch64 helper used by `alu.cpp`, `comparison.cpp`, and
`machine_printer.cpp`. This is the smallest high-value family because the
mapping is byte-for-byte semantic duplication across all three paths and can be
shared without moving record schemas, opcode decisions, or printer formatting.

## Suggested Next

Execute Step 2 by replacing `comparison_scalar_register_view` and
`compare_register_view` with the existing/shared `scalar_register_view`
semantics, then route call sites through the chosen shared AArch64 scalar
integer-view helper. Keep the patch limited to helper reuse and direct call-site
updates; leave FP/storage-view and compare-condition fold-back for later
packets.

## Watchouts

- `integer_type_bit_width` in `machine_printer.cpp` is similar but not identical
  to `integer_scalar_bit_width` because it includes `I1 -> 1`; do not fold that
  into Step 2 unless the call sites are checked explicitly.
- The compare predicate condition-code mapping is a real duplicate, but folding
  it back should be a separate packet so Step 2 does not mix type/view helper
  movement with branch/printer condition spelling.
- `scalar_storage_register_view` depends on the FP scalar view helper; leave it
  in place until the shared integer and FP view helpers are stable.
- Keep BIR facts and `abi::convert_prepared_register` as inputs rather than
  re-deriving shared semantics, and do not move AArch64 record schemas, opcode
  decisions, register-name spelling, scratch selection, or printer formatting
  into shared helper code.

## Proof

Not run; delegated proof was "No build required" for this read-only inventory
plus `todo.md` update. No proof logs were created.
