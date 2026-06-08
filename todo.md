Status: Active
Source Idea Path: ideas/open/128_aarch64_wide_value_owner_post_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Audit i128 Pair Transport, Shift, And Compare Records

# Current Packet

## Just Finished

Step 3: Audit i128 Pair Transport, Shift, And Compare Records completed the
read-only classification of i128 pair transport, shift, and compare record
construction against instruction lowering and machine printing surfaces.

Classification:

- i128 carrier and lane facts for pair transport:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `make_prepared_i128_carrier_transport_record` looks up
  `PreparedI128Carrier`, rejects missing or incomplete facts, copies prepared
  size/alignment/register placement/slot facts into `I128TransportRecord`, and
  converts prepared low/high lane registers through the AArch64 ABI view
  (i128_ops.cpp lines 1064-1165). `make_prepared_i128_copy_transport_record`
  similarly requires the source carrier to already be a complete register pair
  before filling source lane records (lines 1167-1235). The local code validates
  and packages prepared ownership; it does not choose target-neutral carrier
  authority.
- i128 pair operand record construction:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `make_i128_pair_carrier_lane_adapter` requires a prepared register-pair carrier
  with 16-byte total size, 8-byte lanes, low/high roles, lane indexes 0/1, and
  complete register names, then `make_i128_pair_operand_record` copies those lane
  facts into `I128PairOperandRecord` (i128_ops.cpp lines 1268-1309 and
  1414-1447). This is completeness validation over prealloc facts plus AArch64
  register conversion, not late shared-policy selection.
- i128 pair operation record shape:
  `aarch64-codegen-consumption`. `make_prepared_i128_pair_operation_record`
  accepts only I128 add/sub/and/or/xor, maps BIR opcodes to
  `I128PairOperationKind` and lane semantics, resolves prepared value names, and
  attaches prepared result/lhs/rhs pair operands (i128_ops.cpp lines 1313-1513).
  `make_i128_pair_operation_instruction` then publishes low/high defs and uses
  with `MachineOpcode::I128Pair` (lines 2229-2296). The target printer emits
  AArch64 `adds`/`adc`, `subs`/`sbc`, `and`, `orr`, and `eor` spellings from
  that record (lines 681-738). This is target instruction lowering and printing,
  not shared BIR/prealloc policy.
- i128 shift record shape:
  `aarch64-codegen-consumption` with shared scalar-storage consumption for the
  count operand. `make_prepared_i128_shift_record` maps Shl/LShr/AShr to
  `I128ShiftKind` and cross-lane semantics, consumes prepared pair operands for
  result/source, and uses `make_prepared_scalar_operand` over prepared value
  locations/storage plan for the shift count (i128_ops.cpp lines 1633-1778).
  `make_i128_shift_instruction` publishes result pair defs, source pair uses, and
  the shift-count use under `MachineOpcode::I128Shift` (lines 2299-2361). The
  printer owns AArch64 opcode spelling and currently prints only immediate
  counts in the 0..63 subset with `mov`, `lsl`, `extr`, `lsr`, and `asr` (lines
  740-812).
- i128 compare record shape:
  `aarch64-codegen-consumption` with shared carrier and scalar-result storage
  consumption. `lower_i128_pair_operation_instruction` routes compare predicates
  to `lower_prepared_i128_compare_instruction` (i128_ops.cpp lines 2558-2563).
  That adapter requires prepared carrier facts plus value-location/storage-plan
  facts, builds an `I128CompareRecord`, and rejects unselected records
  (comparison.cpp lines 1189-1280). `make_prepared_i128_compare_record` consumes
  prepared scalar result-register placement and prepared i128 pair operands,
  while recording predicate signedness and high-word semantics (i128_ops.cpp
  lines 1781-1915). `make_i128_compare_instruction` publishes the scalar result
  def and pair source uses with `MachineOpcode::I128Compare` (lines 2364-2427).
- i128 compare printing:
  `aarch64-codegen-consumption`. `comparison.cpp` owns the target condition
  spelling helpers for equality and relational predicates (lines 724-783), and
  `print_i128_compare` emits AArch64 `cmp`, `ccmp`, `cset`, conditional
  branches, local labels, and result materialization from the record
  (i128_ops.cpp lines 814-918). The machine printer dispatch surface simply
  forwards `I128PairOperationRecord`, `I128ShiftRecord`, and `I128CompareRecord`
  variants back to the i128 print helpers (machine_printer.cpp lines 2149-2156).
- Instruction record declarations and opcode surface:
  `local-organization-only`. `instruction.hpp` defines the i128 enums and record
  structs used by these target machine records (lines 340-458 and 1038-1309),
  and `InstructionPayload` includes the i128 transport/pair/shift/compare record
  variants (lines 1646-1654). `instruction.cpp` only provides opcode spellings
  such as `i128_pair`, `i128_shift`, and `i128_compare` (lines 196-201). These
  are target MIR organization and diagnostics surfaces, not ownership policy.

Gap candidates:

- No Step 3 shared-policy gap is justified. Pair transport and pair operands
  consume prepared carrier facts, shift count/result facts consume prepared
  scalar value/storage facts, and compare result/register facts consume prepared
  scalar result placement. The remaining decisions are AArch64 record shape,
  machine effects, opcode spelling, label spelling, and printer assembly.
- A precise non-shared support gap candidate exists: shift construction accepts
  register counts and immediate counts up to 127 (`is_supported_i128_shift_count`,
  i128_ops.cpp lines 1705-1710), but `print_i128_shift` rejects register counts
  and immediates outside 0..63 (lines 740-751). Owner boundary:
  AArch64 i128 shift lowering/printing completeness, not BIR/prealloc policy.
  Proof route: add/enable backend cases for i128 shift-by-64-or-more and
  variable-count shifts, then prove selected machine output or an explicit
  unsupported diagnostic. Reject signal: any proposed follow-up that moves
  AArch64 shift opcode spelling or pair-lane register spelling into shared
  BIR/prealloc code.

## Suggested Next

Execute Step 4 from `plan.md`: audit f128 full-width carrier and memory-backed
facts against `src/backend/mir/aarch64/codegen/f128.cpp`,
`src/backend/mir/aarch64/codegen/memory.cpp`, and call-boundary overlap where
needed.

## Watchouts

- This route is analysis-only; do not edit implementation files, tests, or
  build metadata.
- Do not treat `i128_ops.cpp` or `f128.cpp` size as evidence of a boundary gap.
- Do not move AArch64 register spelling, Q-register spelling, lane/shift
  opcode spelling, or helper call assembly into shared BIR/prealloc code.
- Follow-up ideas must be concrete: owner boundary, filenames, proof route, and
  reject signals.
- Step 3 found no shared-policy gap for i128 pair transport, shift, or compare
  records. The only precise live issue is AArch64 shift support completeness for
  variable counts and 64..127 immediates.
- The packet target path `src/backend/mir/aarch64/machine_printer.cpp` is stale;
  the active printer surface is
  `src/backend/mir/aarch64/codegen/machine_printer.cpp`.
- Step 4 should inspect f128 memory-backed carrier printable address handling
  against `memory.cpp`, and f128 carrier publication/consumption against any
  relevant call-boundary overlap.

## Proof

Proof: analysis-only, no build/test run.
