# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3.5
Current Step Title: Receive the checked normalized i32 Mul result

## Just Finished

- Plan Step 5.3.4 receives the exact normalized i32 `Add` result by native
  source ID: its lhs is the admitted selected-global i32 Load result and its
  rhs is typed immediate one. `BinaryNode`/`BinarySpec` now carry only this
  Add/i32 row beside double `FAdd`; Raw and Canonical receipt, source linkage,
  return use, malformed IDs, owner/type/opcode/immediate failures, and rollback
  are covered.

## Suggested Next

- Implement only Plan Step 5.3.5: receive the source-authorized normalized i32
  `LirBinOp Mul` whose lhs is the exact accepted Step 5.3.4 Load-plus-one Add
  result and whose rhs is native immediate two. Extend `BinaryNode`/`BinarySpec`
  only to this Mul/i32 shape and return its result through the accepted
  scalar-i32 return path in the focused fixture; do not infer authority from
  compatibility text.

## Watchouts

- `Mul` is only the i32 Add-result-plus-native-two continuation of the exact
  producer-verified scalar chain. CFG labels and unadvertised inline-assembly
  bindings/payloads remain non-authoritative. Other widths/opcodes, standalone
  or non-Add lhs, non-two/SSA rhs, and presentation-derived facts remain
  fail-closed.

## Proof

- Plan Step 5.3.4 passed:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
  The focused subset passed; `test_after.log` is the proof log.
- Plan Step 5.3.5 must run a fresh build and the same focused selection:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'`.
