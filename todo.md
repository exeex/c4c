Status: Active
Source Idea Path: ideas/open/611_rv64_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement First Supported Terminator Consumer Rule

# Current Packet

## Just Finished

Step 2 from `plan.md` implemented the first supported RV64 prepared terminator
consumer rule for integer scalar fused-compare conditional branches.

The RV64 object terminator path now accepts prepared `branch_condition`
records with `kind=fused_compare`, explicit predicate/lhs/rhs values, and
operand materialization through existing prepared homes or immediates. The
rule normalizes RV64 GPR integer branch predicates across `i1`/`i8`/`i16`/
`i32`/`i64`, emits the direct conditional branch, and remains fail-closed when
normalization or operand materialization is unavailable.
The Step 2 helper is locally guarded to matching GPR integer lhs/rhs types so
floating and pointer compare branches remain outside this integer packet.

Focused direct object probes that now compile to RV64 objects:
- `src/20030403-1.c`
- `src/961017-2.c`
- `src/pr48197.c`
- `src/pr80501.c`

Focused negative probes that still fail closed for branch authority/freshness:
- `src/20000314-3.c`: `unsupported_branch_stack_load_authority`
- `src/20060910-1.c`: `unsupported_branch_stack_load_source_freshness`
- `src/930930-1.c`: `unsupported_branch_stack_load_source_freshness`
- `src/990127-1.c`: `unsupported_branch_stack_load_source_freshness`

Focused backend coverage was updated to assert `ugt i64` prepared fused
compare branch object emission and to keep pointer/malformed branch shapes in
the fail-closed bucket.

## Suggested Next

Step 3 diagnostic packet:

Refresh the `unsupported_terminator_fragment` residual after the integer
scalar consumer rule. Split remaining rows into adjacent authorized terminator
families versus branch operand authority, branch stack-source freshness,
floating compare branches, pointer compare branches, return-ABI, and generic
instruction-fragment owners before broadening any code.

## Watchouts

- Do not infer missing branch operands from final layout.
- Keep branch stack-source freshness and prepared authority publication out of
  scope.
- Do not touch expectations, unsupported markers, allowlists, runtime,
  timeout/accounting, move-bundle, ABI, or generic instruction-fragment work.
- Keep `src/20030910-1.c` and `src/920618-1.c` as diagnostic candidates for a
  later floating compare branch packet, not evidence for this integer scalar
  slice.
- Keep `src/20000314-3.c`, `src/20001017-1.c`, `src/20050125-1.c`,
  `src/20080519-1.c`, `src/20140828-1.c`, `src/loop-2e.c`,
  `src/pr39100.c`, `src/20060910-1.c`, `src/930930-1.c`, and
  `src/990127-1.c` as negative authority/freshness rows.
- The direct probe path is `build/c4cll`, not `build/src/apps/c4cll`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. Test subset: `^backend_`. Proof log: `test_after.log`.

Additional focused probe:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`

Result: passed before the full backend subset rerun.
