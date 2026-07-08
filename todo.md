Status: Active
Source Idea Path: ideas/open/611_rv64_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Terminator-Fragment Residual Diagnostics

# Current Packet

## Just Finished

Step 1 from `plan.md` refreshed the RV64 backend-object
`unsupported_terminator_fragment` residual diagnostics and split the current
terminator bucket from adjacent branch-authority buckets.

Current refreshed count:
- `71` unique `build/rv64_gcc_c_torture_backend/*/case.log` rows report
  `unsupported_terminator_fragment: BIR terminator requires unsupported RV64
  object lowering`.

Representative supported RV64 consumer candidates:
- `src/20030403-1.c`: `entry` fused compare branch
  `ugt i64 %t1, 2147483647`, with prepared homes for `%t1` and `%t3` and
  scalar-layout local load authority.
- `src/20030910-1.c`: `entry` fused compare branch
  `ne double %t4, 0x40091EB851EB851F`, with `%t4` in `ft0`, condition `%t5`
  in `s1`, and no branch stack-source authority dependency.
- `src/920618-1.c`: `entry` fused compare branch
  `sle float 0x00000000, %t0`, with `%t0` in `ft0`, condition `%t1` in `t0`,
  and no branch stack-source authority dependency.
- `src/pr48197.c`: multiple integer fused compare branches beginning with
  `eq i64 8, 4`; this is useful positive breadth after the first rule, but is
  not the smallest seed because the file has many later branches.

Residual shape split inside the 71 terminator rows by first visible prepared
branch condition:
- `24` `i64` fused compare rows, `22` `double`, `11` `i32`, `10` `ptr`, and
  `4` `float`.
- Opcode/type leaders are `ne double` (`11`), `ne ptr` (`7`), `eq i64` (`7`),
  and `ugt i64` (`6`).
- No row in the 71-row terminator bucket lacked a prepared
  `branch_condition`; all sampled rows are prepared `kind=fused_compare` with
  explicit condition/lhs/rhs evidence.

Separated adjacent first owners in the refreshed backend scan:
- Branch operand/layout authority, not Step 2 terminator consumer:
  `src/20000314-3.c`, `src/20001017-1.c`, `src/20050125-1.c`,
  `src/20080519-1.c`, `src/20140828-1.c`, `src/loop-2e.c`,
  `src/pr39100.c` report `unsupported_branch_stack_load_authority`.
- Branch stack-source freshness, not Step 2 terminator consumer:
  `src/20060910-1.c`, `src/930930-1.c`, and `src/990127-1.c` report
  `unsupported_branch_stack_load_source_freshness`.
- Prepared control-flow fact first owners: none observed in the refreshed
  backend logs.
- Downstream non-terminator first owners among the current 71-row terminator
  bucket: none observed; rows that already moved to other buckets are outside
  this packet's terminator count.

## Suggested Next

Step 2 implementation packet:

Objective: implement the first RV64 prepared terminator consumer rule for
explicitly authorized fused compare conditional branches, starting with
register/immediate `i64` scalar predicates such as
`src/20030403-1.c` (`ugt i64 %t1, 2147483647`) and proving nearby
integer-branch breadth (`src/961017-2.c`, `src/pr48197.c`,
`src/pr80501.c`) without touching floating, pointer, branch-source
publication, or return-ABI handling.

Owned files should include the RV64 prepared compare/terminator consumer code
and `todo.md`. Keep the rule semantic: require the prepared
`branch_condition` to be `kind=fused_compare` with explicit predicate/lhs/rhs
and require operand materialization through existing prepared homes or
immediates. Preserve fail-closed behavior for missing homes, stack-source
freshness, pointer branch operand authority, and missing/ambiguous prepared
control-flow facts.

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Watchouts

- Do not infer missing branch operands from final layout.
- Keep branch stack-source freshness and prepared authority publication out of
  scope.
- Do not touch expectations, unsupported markers, allowlists, runtime,
  timeout/accounting, move-bundle, ABI, or generic instruction-fragment work.
- Do not use `src/20030910-1.c` or `src/920618-1.c` as the first
  implementation seed unless the packet is explicitly broadened to floating
  compare branch lowering; they are good positive candidates for later breadth,
  but not the smallest integer scalar seed.
- Keep `src/20000314-3.c`, `src/20001017-1.c`, `src/20050125-1.c`,
  `src/20080519-1.c`, `src/20140828-1.c`, `src/loop-2e.c`,
  `src/pr39100.c`, `src/20060910-1.c`, `src/930930-1.c`, and
  `src/990127-1.c` as negative authority/freshness rows.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. Test subset: `^backend_`. Proof log: `test_after.log`.
