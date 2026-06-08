Status: Active
Source Idea Path: ideas/open/128_aarch64_wide_value_owner_post_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit f128 Carrier And Memory-Backed Facts

# Current Packet

## Just Finished

Step 4: Audit f128 Carrier And Memory-Backed Facts completed the read-only
classification of f128 full-width carrier and memory-backed transport facts
against `f128.cpp`, `memory.cpp`, and relevant `calls.cpp` overlap.

Classification:

- f128 full-width carrier facts:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `prepared_f128_full_width_carrier_facts` accepts only complete prepared f128
  carriers with `FullWidthRegister`, F128 source type, 16-byte size/alignment,
  Vreg/vector class, contiguous width 1, and a prepared register name
  (f128.cpp lines 234-272). `make_f128_register_operand` consumes those facts,
  parses the prepared register through the AArch64 ABI view, and re-views it as
  a Q register while preserving prepared value id/name, register class/bank, and
  contiguous width in the `RegisterOperand` (lines 1658-1683). This validates
  prepared carrier ownership and performs target register conversion; it does
  not select the carrier authority.
- f128 carrier transport record construction:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `make_prepared_f128_carrier_transport_record` finds the prepared f128 carrier,
  rejects missing or incomplete carriers, copies prepared value, kind,
  size/alignment, register placement, slot id, stack offset, and occupied
  register names into `F128TransportRecord`, and then dispatches by prepared
  carrier kind (f128.cpp lines 1781-1857). Full-width carriers must pass
  `prepared_f128_full_width_carrier_facts` before getting a Q-register operand;
  memory-backed carriers must pass `prepared_f128_memory_backed_carrier_facts`
  before the record is accepted. The record is a target machine payload over
  prepared authority, not a new shared transport decision.
- f128 memory-backed carrier facts:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `prepared_f128_memory_backed_carrier_facts` accepts only complete prepared f128
  carriers with `MemoryBacked`, F128 source type, 16-byte size/alignment, and
  prepared slot id plus stack offset (f128.cpp lines 243-294). Its overload for
  `F128TransportRecord` cross-checks the record against the original
  `source_carrier`, then verifies the copied slot id and stack offset still
  match the prepared facts (lines 297-317). This is explicit no-gap evidence:
  AArch64 refuses to invent stack homes or offsets for f128 memory-backed
  carriers.
- f128 memory operand overlap:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `lower_f128_transport_instruction` requires prepared function state, prepared
  f128 carrier tables, and prepared addressing; it builds the load/store memory
  operand through `make_prepared_memory_operand_record`, resolves prepared
  frame-slot offsets and pointer bases when needed, then calls
  `make_prepared_f128_carrier_transport_record` with the prepared carrier value
  name and memory record (memory.cpp lines 2910-3067). The memory-backed helper
  later reconstructs a `MemoryOperand` from prepared slot/offset/size/alignment
  via `make_prepared_frame_slot_memory_operand` and attaches stored/result value
  ids according to load/store direction (f128.cpp lines 362-385). Addressing and
  stack-home authority remain prepared memory/prealloc facts.
- f128 call-boundary overlap:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `F128CarrierCallOperandOwner::is_complete_full_width` performs the same
  completeness gate for call moves: full-width f128 carrier, no missing facts,
  16-byte size/alignment, Vreg/vector, contiguous width 1, and register name
  present (calls.cpp lines 890-920). Argument and result call-boundary moves
  require that complete prepared carrier and also check it agrees with prepared
  value homes before creating Q-register operands (calls.cpp lines 2732-3039 and
  4356-4495). Stack call-argument stores fall back to
  `make_prepared_f128_carrier_transport_record` with prepared carrier facts and
  a prepared destination memory operand (lines 3887-3917). This overlap consumes
  and cross-validates prepared call/memory/carrier facts; it does not choose
  target-neutral carrier placement late.

Gap candidates:

- No Step 4 shared-policy gap is justified. Full-width f128 carriers,
  memory-backed carriers, memory operands, and call-boundary f128 moves all
  consume prepared facts and reject missing or inconsistent authority. The only
  duplication observed is local completeness validation in `f128.cpp` and
  `calls.cpp`; the validation checks prepared facts and then performs AArch64
  Q-register/memory-record conversion, so it is not evidence that shared
  BIR/prealloc policy is being rediscovered.
- No precise shared-policy follow-up candidate is warranted from Step 4. A valid
  future candidate would need to show AArch64 selecting f128 register-vs-memory
  carrier ownership, stack slot, stack offset, or call-boundary publication
  authority without prepared input. Current evidence shows the opposite.

## Suggested Next

Execute Step 5 from `plan.md`: audit f128 helper resources, preservation
checks, transport construction, printable-address handling, Q-register spelling,
and printer-facing record fields.

## Watchouts

- This route is analysis-only; do not edit implementation files, tests, or
  build metadata.
- Do not treat `i128_ops.cpp` or `f128.cpp` size as evidence of a boundary gap.
- Do not move AArch64 register spelling, Q-register spelling, lane/shift
  opcode spelling, or helper call assembly into shared BIR/prealloc code.
- Follow-up ideas must be concrete: owner boundary, filenames, proof route, and
  reject signals.
- Step 4 found no shared-policy gap for f128 full-width carrier or memory-backed
  carrier facts. The no-gap evidence depends on prepared carrier, prepared
  addressing, prepared value-home, and prepared call-boundary facts being
  present before AArch64 record construction.
- Step 5 should keep printable-address materialization and Q-register spelling
  local unless the code proves target-neutral helper/preservation policy is
  being selected late.

## Proof

Proof: analysis-only, no build/test run.
