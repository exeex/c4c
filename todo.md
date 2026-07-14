# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3.4
Current Step Title: Receive the checked normalized i32 Add result

## Just Finished

- Plan Step 6.2 extends the existing source-result registry receipt to the
  exact i64 `ordinary_results[0]` row: valid current-function `LirValueId`,
  `Output` role, constraint index zero, and one same-ID i64 direct-global
  `Store`. The Step 6.1 i32 row remains accepted. Raw and Canonical tests now
  cover both widths plus missing/invalid/duplicate bindings, role/index/type,
  unknown/cross-function/mismatched Store uses, and transactional rollback.

## Suggested Next

- Implement only Plan Step 5.3.4: receive the source-authorized normalized i32
  `LirBinOp Add` with the already admitted selected-global i32 Load result as
  lhs and native immediate one as rhs. Extend `BinaryNode`/`BinarySpec` only to
  this Add/i32 shape and return the result through the accepted scalar-i32
  return path in the focused receipt fixture; do not recover authority from
  compatibility text.

## Watchouts

- Step 6.3 had no executable terminator/inline-assembly row: CFG labels remain
  non-authoritative and all unadvertised inline-assembly bindings/payloads stay
  fail-closed. For Step 5.3.4, accept neither `Mul` nor any other binary
  opcode/width, SSA rhs, nonselected immediate, presentation-derived operand,
  compound/complex/vector/pointer/object/logical-helper/builtin form, or a
  malformed/cross-function result/use. The accepted inline-assembly i32/i64
  and double-`FAdd` rows are regression neighbors, not authority for this row.

## Proof

- Plan Step 6.2 passed:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
  The focused subset passed; `test_after.log` is the proof log.
- Plan Step 5.3.4 must run a fresh build and the same focused selection:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'`.
