Status: Active
Source Idea Path: ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory accepted intrinsic families and carrier fields

# Current Packet

## Just Finished

Step 1, "Inventory accepted intrinsic families and carrier fields", completed.

Existing scalar `fabs` path:

- BIR authority is `bir::CallInst::intrinsic` with
  `bir::IntrinsicOperation{family=ScalarFpUnary, operation=FAbs,
  operand_type, result_type, has_side_effects=false}`. The call also carries
  structured operand/result facts through `result`, `args`, `arg_types`,
  `arg_abi`, `return_type`, and `result_abi`; `callee` remains display/source
  context, not authority.
- Prepared authority is `PreparedIntrinsicCarrier{carrier_kind, family,
  operation, block_index, inst_index, operand_type, result_type, operand,
  result, operand_value_name, result_value_name, has_side_effects,
  requires_feature, has_prepared_call_plan, missing_required_facts}`. Current
  completion requires exactly one operand, a result, matching F32/F64 types,
  and a prepared call plan with one argument and one result.
- Prepared-printer proof is limited to complete carriers and prints family,
  operation, block/inst indexes, operand/result types, side-effect and feature
  booleans, prepared-call-plan presence, optional source callee, and
  operand/result value names. Missing carriers print only `missing fact=...`.

Minimum accepted representatives for the new carrier route:

- CRC representative: accept one AArch64 CRC32 scalar intrinsic, preferably
  `llvm.aarch64.crc32w`-shaped semantics: family `Crc`, operation `Crc32W`,
  required feature `crc`, accumulator/result type I32, data operand type I32,
  unsigned 32-bit data width, two ordered operands, one I32 result, no memory
  operand, no immediate operand, no side effects, and GPR register/home
  authority for accumulator, data, and result.
- Vector memory representative: accept one AArch64 vector load intrinsic,
  preferably a 128-bit integer vector load such as a `vld1`/load-one-vector
  representative: family `VectorMemory`, operation `VectorLoad`, required
  vector/NEON feature, element type I8, element width 1 byte, lane count 16,
  total width 16 bytes, result vector type, pointer memory operand with
  address space, size, alignment, volatility, base/offset identity, one vector
  result, side-effect/read classification, and Vreg result home authority plus
  pointer GPR/address authority.
- Vector operation representative: accept one AArch64 vector arithmetic
  intrinsic, preferably 16xI8 add-shaped semantics: family `VectorOperation`,
  operation `VectorAdd`, required vector/NEON feature, element type I8,
  element width 1 byte, lane count 16, total width 16 bytes, signedness or
  unsignedness as applicable, two vector operands, one vector result, no memory
  operand, no immediate operand, no side effects, and Vreg operand/result
  register/home authority.

Missing BIR/prepared fields or enum values:

- `bir::IntrinsicFamilyKind` only has `None` and `ScalarFpUnary`; add scoped
  values for `Crc`, `VectorMemory`, and `VectorOperation`.
- `bir::IntrinsicOperationKind` only has `None` and `FAbs`; add scoped
  operations for the three representatives, with names that do not encode final
  assembly spelling as authority.
- `bir::IntrinsicOperation` lacks required feature identity, operand count
  beyond unary, per-operand roles/types, vector element type, element width,
  lane count, total vector width, signedness, memory contract, immediate
  contract, and explicit read/write side-effect classification.
- `bir::TypeKind` has scalar and pointer kinds only; the accepted vector cases
  need either vector-specific type facts on the intrinsic operation or a new
  vector type carrier that records element kind plus lane/width facts.
- `PreparedIntrinsicCarrier` mirrors only the scalar-unary shape: one
  `operand`, one `operand_type`, one operand value name, one result, one
  boolean `requires_feature`, and no feature kind/name. It needs
  family-specific structured facts for feature, ordered operands and roles,
  vector lane/type/width/signedness, memory operand/address facts, immediate
  facts, result identity, side-effect/read-write classification, and register
  authority.
- Prepared register authority currently exists nearby in `PreparedValueHome`,
  `PreparedStoragePlanValue`, and `PreparedCallArgumentPlan` /
  `PreparedCallResultPlan`, including GPR/FPR/Vreg banks, register classes,
  placements, contiguous widths, and value ids. The intrinsic carrier must bind
  to those structured facts directly; ordinary `PreparedCallPlan` presence
  alone is not enough.

Fail-closed cases to preserve:

- x86-only intrinsic families such as `llvm.x86.aesni.*`, SSE/AVX, CLMUL, and
  x86/F128 fabs shapes must not become AArch64 CRC/vector carriers.
- Incomplete CRC/vector facts must stay missing: missing intrinsic operation,
  unsupported family/operation, missing required feature, missing or mismatched
  lane/type/width facts, wrong operand count, missing result, missing memory
  operand for vector memory, unexpected memory operand for pure vector op/CRC,
  missing immediate where required, unsupported side-effect contract, missing
  prepared call plan, and missing register/home authority.
- Intrinsic spelling, ordinary call plans, archived scratch registers, and
  final assembly text remain non-authoritative.

## Suggested Next

Start Step 2 by adding BIR-only semantic carrier facts for the three accepted
representatives above, plus BIR notes/fail-closed tests for unsupported
x86-only spellings and incomplete facts. Keep prepared carrier completion
missing until Step 3 wires the new BIR facts into `PreparedIntrinsicCarrier`.

## Watchouts

- Do not select or print AArch64 CRC/vector machine records in this dependency
  route.
- Do not use intrinsic-name matching, ordinary call plans alone, archived
  scratch registers, or final assembly text as carrier authority.
- Keep unsupported x86-only and incomplete-fact paths fail-closed.
- Avoid adding only a boolean feature flag for CRC/vector: the selector will
  need a concrete feature identity such as `crc` or vector/NEON, not just
  `requires_feature=yes`.
- Avoid preserving the scalar-unary one-operand shape behind generic names;
  CRC needs two scalar operands, vector memory needs a memory operand, and
  vector operations need lane/type/width facts plus multiple vector operands.

## Proof

Proof command:

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(prepared_printer|aarch64_instruction_dispatch|aarch64_machine_printer|lir_to_bir_notes)'; } 2>&1 | tee test_after.log`

Additional check:

`git diff --check`
