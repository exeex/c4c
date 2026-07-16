# Current Packet

Status: Active
Source Idea Path: ideas/open/833_lir_truthiness_lhs_parameter_authority_completion.md
Source Plan Path: plan.md
Current Step ID: 2b
Current Step Title: Produce and verify the evidenced authority relation

## Just Finished

- Step 2b populated `LirCmpOp.truthiness_lhs_parameter_authority` at only the
  evidenced `emit_binary_rval_operand` integer `BinaryOp::Ne` producer. It
  requires preserved SSA LHS, immediate integer zero RHS, and one exact
  current-function `DirectScalar` native definition with matching value,
  `LirTypeRef`, and owner; it copies that definition's value, parameter index,
  type, owner, ABI, and the `TruthinessComparisonLhs` role.
- Updated the nearby generated-path fixture to use `if (value != 0)`, so its
  existing positive and missing/foreign/owner/type/ABI/role/predicate/RHS/
  duplicate malformed checks now exercise this binary producer rather than the
  already-covered `to_bool_operand` route.
- Fresh build succeeded. The exact 13-case proof now passes 9/13, including
  `930719-1`; none fail at the missing truthiness-authority verifier relation.
  The remaining four failures are visible aggregate-owner errors in
  `20090113-2`, `comp-goto-1`, `pr51323`, and `pr88714`.

## Suggested Next

- Supervisor: evaluate Step 2b proof and route Step 3 only if the remaining
  aggregate-owner failures are accepted as out of scope for this relation.

## Watchouts

- Do not re-open the disproven `UnaryOp::Not` or string-only `to_bool` route,
  and do not absorb the four aggregate-owner failures. The nearest whole
  frontend LIR test aborts before completion at the same pre-existing
  `LIR-owned aggregate function type requires a matching module owner` error;
  its selected generated-path coverage is compiled but cannot report green
  until that separate route is repaired.

## Proof

- `cmake --build --preset default` — passed (existing deprecation warnings).
- `ctest --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'`
  — failed before completion at the pre-existing aggregate-owner runtime error.
- `ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_(20090113_2|930719_1|931012_1|950512_1|961112_1|comp_goto_1|pr23604|pr28289|pr37780|pr43385|pr46909_2|pr51323|pr88714)_c)$' > test_after.log`
  — 9 passed / 4 failed only at the aggregate-owner error; canonical log:
  `test_after.log`.
