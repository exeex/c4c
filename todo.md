# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3.3
Current Step Title: Receive the checked downstream double FAdd source chain

## Just Finished

- Plan Step 5.3.2 received the resolved fixed-void native-double `LirCallOp`
  into one direct Raw-BIR `Call` with its native `F64` result, current-function
  source identity, and exact resolved `LinkNameId` target. Raw and Canonical
  receipt plus missing/conflicting callee, result identity, return/signature,
  and alternate-carrier rollback coverage now pass; scalar `FAdd` remains
  unimported.

## Suggested Next

- Execute Plan Step 5.3.3 only: receive the exact checked downstream
  `LirBinOp{result, opcode=FAdd, type_str=double, lhs=<Step 5.3.2 call
  result>, rhs=<current-function double SSA>}` into one typed Raw-BIR binary
  instruction/result. Add the Binary builder payload/spec and reachable
  verifier rule for this source-ID/type/opcode/operand linkage, then prove Raw
  and Canonical receipt plus transactional rejection of missing, duplicate,
  cross-owner, wrong-type, non-`FAdd`, non-SSA, or malformed-result authority
  with `backend_lir_to_bir_interface` and `frontend_lir_call_type_ref`.

## Watchouts

- This packet admits neither a general binary family nor a new call form: keep
  every integer, other floating opcode, unary, literal/presentation-derived,
  compound, complex, vector, pointer/object, logical-helper, builtin, and
  nonchecked operand/result row fail-closed. Do not infer operands, types, or
  opcode from text; do not receive casts, compares, selects, returns, or any
  further downstream use.

## Proof

- Plan Step 5.3.2 passed:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
  The matching `test_before.log` is green.
- Plan Step 5.3.3 proof:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
