Status: Active
Source Idea Path: ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Special Carrier Policy Duplication

# Current Packet

## Just Finished

Completed `plan.md` Step 1 mapping for AArch64 i128/f128 special-carrier policy duplication.

Prepared authority already available:

- `PreparedI128Carrier` / `PreparedF128Carrier` own carrier kind, value ids/names,
  register bank/class, occupied registers, register placement, frame-slot/stack
  offsets, sizes/alignments, missing-fact diagnostics, and i128 low/high lane
  roles, lane indexes, lane widths, and lane register/slot facts.
- `PreparedI128RuntimeHelper` owns helper family/kind/callee identity,
  low/high lane bindings, ABI register bindings, marshaling moves, result
  ownership, resource policy, ABI policy, selected-call ownership,
  live-preservation policy, scalar ownership for conversion helpers, memory
  return placeholders, and call clobbers.
- `PreparedF128RuntimeHelper` owns helper family/kind/callee identity,
  full-width carrier bindings, ABI register bindings, marshaling moves,
  arithmetic/comparison/cast result ownership, scalar result/operand ownership,
  comparison result consumption, resource policy, ABI policy,
  selected-call ownership, live-preservation policy, and call clobbers.
- Existing lookup surfaces are `find_prepared_i128_carriers`,
  `find_prepared_f128_carriers`, `find_prepared_i128_runtime_helpers`,
  `find_prepared_f128_runtime_helpers`, `find_prepared_i128_carrier`,
  `find_prepared_f128_carrier`, prepared value-home lookup, storage-plan lookup,
  and scalar prepared operand/result helpers.

AArch64-local emission policy to keep local:

- i128 pair/lane instruction selection, opcode-to-pair/shift/compare record
  enums, carry/borrow/high-word lane sequencing, shift-count admissibility, and
  final pair/lane `MachineOpcode` records.
- AArch64 register-view conversion from prepared register names into printable
  `x`, `w`, `s`, `d`, and `q` views.
- f128 Q/vector rendering, reserved scratch register use, memory address
  spelling/materialization, relocation spelling, and load/store instruction
  choice.
- Helper-boundary record construction, printable marshal/unmarshal instruction
  lines, final `bl` emission, scalar compare `cmp`/`cset` rendering, machine
  defs/uses/clobbers, and target diagnostics for missing prepared facts.

Duplication to remove in implementation packets:

- `i128_ops.cpp` still rebuilds i128 pair/transport operands from carriers in
  multiple AArch64-local helpers and repeats carrier completeness checks around
  `RegisterPair`, lane size, lane role/index, and lane register presence before
  pair/lane emission.
- `i128_ops.cpp` still revalidates div/rem helper result ownership, ABI policy,
  resource policy, selected-call ownership, live preservation, clobbers, and
  lane bindings while building and printing the helper boundary, even though
  those facts are already prepared helper authority.
- `f128.cpp` still rebuilds full-width carrier transport and helper operands
  from prepared carriers, repeats `FullWidthRegister` / `MemoryBacked`
  completeness checks, and locally cross-checks helper carrier bindings against
  prepared carriers before vector/helper emission.
- `f128.cpp` still revalidates arithmetic/comparison/cast ABI transitions,
  result ownership, scalar ownership, resource policy, selected-call ownership,
  live preservation, clobbers, and comparison result consumption while building
  and printing helper boundaries.

## Suggested Next

First executable implementation packet: Step 2 should narrow to
`src/backend/mir/aarch64/codegen/i128_ops.cpp` and make the non-helper i128
pair/shift/compare path consume a single prepared carrier/lane adapter for
result/lhs/rhs operands. Keep opcode-to-pair/shift/compare selection, lane
sequence emission, shift-count handling, and final machine-record construction
local. Suggested proof for that packet: supervisor-selected focused AArch64
i128 pair/lane subset plus `cmake --build --preset default > test_after.log
2>&1`.

## Watchouts

Do not move AArch64 pair/lane, vector, memory-address spelling,
helper-boundary records, scratch-register use, scalar compare rendering, or
final machine-record emission into prepared code. Treat prepared facts as the
source of policy truth, but keep target-local validation/diagnostics only where
they guard printability or machine-record emission. The mapping did not find a
missing prepared-authority surface for the first i128 pair/lane packet.

## Proof

Delegated proof passed: `cmake --build --preset default > test_after.log
2>&1`. Proof log: `test_after.log`.
