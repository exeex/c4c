Status: Active
Source Idea Path: ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Scalar Cast Contract

# Current Packet

## Just Finished

Step 2 - Define The Scalar Cast Contract completed for the residual scalar
F32/F64 cast route.

Accepted predicate name:
`rv64_prepared_scalar_f32_f64_cast_contract`.

Targeted sources read:

- `src/backend/mir/riscv/codegen/object_emission.cpp` helpers
  `fragment_for_prepared_floating_cast`,
  `fragment_for_prepared_int_to_fp_cast`, `fragment_for_prepared_cast`, and
  the `unsupported_floating_cast` rejection text.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` fixtures and
  tests for prepared FPR `fpext`/`fptrunc`, formal FPR casts, immediate
  `sitofp`, and unsupported floating-cast rejection.

Minimal accepted contract:

- Opcode is one of the scalar F32/F64 RV64 hardware conversion opcodes:
  existing `FPExt F32 -> F64`, existing `FPTrunc F64 -> F32`, existing
  `SIToFP`/`UIToFP` from `I32`/`I64` to `F32`/`F64`, or new
  `FPToSI`/`FPToUI` from `F32`/`F64` to `I32`/`I64`.
- Source and destination scalar types are only `F32`, `F64`, `I32`, or `I64`
  in the opcode-compatible positions above.
- FPR-width casts require source and destination values to have coherent FPR
  register homes.
- FPR-to-integer casts require a coherent FPR register home for the source and
  a coherent GPR register home for the destination.
- Integer-to-FPR casts keep the existing authority: coherent FPR register home
  for the destination, GPR source when present, or existing integer immediate
  materialization through the integer-immediate path.
- The implementation must not synthesize a missing value home, bank, opcode, or
  conversion direction from the row name or from nearby instructions.

Accepted Step 1 rows:

- `src/cvt-1.c`: accept only the prepared F64 -> I64 `FPToSI` register
  conversion shapes with FPR source homes and GPR destination homes.
- `src/pr66233.c`: accept only the prepared F32 -> I32 `FPToUI` register
  conversion shape with an FPR source home and GPR destination home.

Rejected or split variants:

- Missing cast opcode: reject with an owner-specific scalar-cast diagnostic;
  object emission must not infer opcode from source/destination types.
- Missing source bank or destination bank: reject before lowering. FPR homes
  must decode as FPR registers and GPR homes must decode as GPR registers for
  the accepted direction.
- Missing value home: reject before lowering, except for the already-supported
  integer immediate source path in int-to-FPR casts.
- Unsupported scalar type: reject `I1`, `I8`, `I16`, pointer, aggregate, vector,
  VRM, and non-scalar types from this contract.
- Unsupported direction: reject same-type FPR casts, FPR-to-FPR directions
  other than F32/F64 width conversion, integer widths other than I32/I64,
  FPR-to-stack destinations, GPR-to-FPR shapes outside existing int-to-FPR
  authority, and bitcast-style FP/integer reinterpretation.
- F128/helper-dependent casts: quarantine out of scope. F128 routes require
  helper ABI/carrier authority and are not part of this F32/F64 object-lowering
  route.
- Contradictory facts: reject when the prepared home bank, register identity,
  value type, or opcode direction disagree. Do not repair by copying through a
  stack slot or by choosing a fallback register silently.

FP-immediate decision for `src/920618-1.c`:

- Split before implementation. This row is `FPTrunc` from a typed F64 immediate
  to an F32 FPR register. Existing targeted object-emission evidence has
  integer-immediate materialization contracts but no prepared FP-immediate
  materialization contract. RV64 object emission must not infer a hidden source
  FPR home or invent floating-constant rematerialization in this packet. A
  separate producer/prepared-fact boundary is needed before this row can be
  admitted.

## Suggested Next

Delegate Step 3 - Implement Or Split The Accepted Boundary. Implement the
FPR-to-I32/I64 `FPToSI`/`FPToUI` register conversion boundary for explicit
prepared homes, and keep the `920618-1.c` FP-immediate case split unless a
separate producer/prepared contract is created first.

## Watchouts

The next implementation packet should add diagnostics for fail-closed shapes,
not broaden the old generic `unsupported_floating_cast` text. Do not add
floating immediate materialization under RV64 object emission without a
prepared-fact contract. Keep F128/helper ABI, local-memory, aggregate/byval,
stack-frame, branch/select, call/return, and `conversion.c` work out of this
route.

## Proof

`git diff --check -- todo.md`
