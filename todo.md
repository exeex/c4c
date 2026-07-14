# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3.5
Current Step Title: Receive the checked normalized i32 Mul result

## Just Finished

- Plan Step 5.3.5 receives the exact normalized i32 `Mul` result by native
  source ID: its lhs is the admitted Load-plus-one Add result and its rhs is
  typed immediate two. `BinaryNode`/`BinarySpec` now carry only this Mul/i32
  row beside double `FAdd` and Add/i32; Raw and Canonical receipt preserve the
  full Load -> Add(one) -> Mul(two) -> scalar-i32 Return chain, source linkage,
  malformed IDs, owner/type/opcode/immediate failures, and rollback.

## Suggested Next

- Plan Step 6.1: receive the checked i32 output-only inline-assembly binding
  and its exact selected-global i32 Store use, without widening ordinary scalar
  binary receipt.

## Watchouts

- `Mul` remains only the i32 Add-result-plus-native-two continuation of the
  exact producer-verified scalar chain. CFG labels and unadvertised
  inline-assembly bindings/payloads remain non-authoritative. Other
  widths/opcodes, standalone or non-Add lhs, non-two/SSA rhs, and
  presentation-derived facts remain fail-closed.

## Proof

- Plan Step 5.3.5 passed:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
  The focused subset passed; `test_after.log` is the proof log.
