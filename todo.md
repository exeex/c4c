# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3
Current Step Title: Take subsequent checked ordinary rows one at a time

## Just Finished

- Plan Step 5.3: received the selected deferred i64 intrinsic-result
  `LirCastOp Trunc` row into a separately tagged Raw-BIR cast payload with an
  exact `i64 -> i32` edge, the current-function `Cttz`/`Ctlz`/`Ctpop` result
  as its sole operand, and a source-backed owning result. The builder,
  importer, and verifier fail closed for every other cast shape, operand
  authority, result collision, foreign/non-intrinsic source, or malformed
  Raw-BIR payload; Raw and Canonical receipt/rejection coverage was added.

## Suggested Next

- Supervisor selects the next Step 5.3 ordinary row; this packet does not
  admit broader scalar casts or downstream uses of the truncation result.

## Watchouts

- This packet admits only the i64 intrinsic-result to i32 `Trunc` edge.
  Broader scalar casts (integer width changes, FPTrunc/FPExt,
  SIToFP/UIToFP, FPToSI/FPToUI), scalar binary/compare/select/abs rows, direct
  floating calls, and opaque inline assembly remain separately scoped. Other
  intrinsic kinds and unsupported direct/indirect/variadic/ABI/aggregate/object
  forms remain fail-closed; no text recovery or partial module publication.

## Proof

- `cmake --build --preset default` followed by
  `ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$' > test_after.log`
  passed for this Step 5.3 packet; `test_after.log` is the preserved proof log.
