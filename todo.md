# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3
Current Step Title: Take subsequent checked ordinary rows one at a time

## Just Finished

- Plan Step 5.3.4 receives the exact normalized i32 `Add` result by native
  source ID: its lhs is the admitted selected-global i32 Load result and its
  rhs is typed immediate one. `BinaryNode`/`BinarySpec` now carry only this
  Add/i32 row beside double `FAdd`; Raw and Canonical receipt, source linkage,
  return use, malformed IDs, owner/type/opcode/immediate failures, and rollback
  are covered.

## Suggested Next

- Select the next row only after an explicit source-authorized ordinary receipt
  is evidenced; do not infer semantics from compatibility text.

## Watchouts

- At the Step 5.3 selection checkpoint, CFG labels and unadvertised
  inline-assembly bindings/payloads remain non-authoritative. The accepted i32
  Add is only Load-plus-native-one; `Mul`, other widths/opcodes, SSA rhs, and
  presentation-derived facts remain closed.

## Proof

- Plan Step 5.3.4 passed:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
  The focused subset passed; `test_after.log` is the proof log.
