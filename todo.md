Status: Active
Source Idea Path: ideas/open/370_aarch64_short_circuit_control_publication_regression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Short-Circuit Regression Boundary

你該做baseline sanity check了

# Current Packet

## Just Finished

Step 1 localized the `00196` short-circuit regression while preserving the
idea 366 representative `00112` proof.

`tests/c/external/c-testsuite/src/00196.c` line 21 is the first failing
expression:

```c
printf("%d\n", fred() && (1 + joe()));
```

The expected output for that expression is `fred` then `0`. The current AArch64
output is `fred` then `1`; it does not print `joe` for this expression in the
current generated run, so the first bad fact is stale RHS-derived value
publication at the join, not RHS call execution.

Semantic BIR is still correct. It branches on `%t50 = bir.ne i32 %t49, 0`;
`logic.skip.52` reaches the join with incoming `0`, and only `logic.rhs.51`
contains the `joe()` call, `%t56 = bir.add i32 1, %t55`, and `%t57 = bir.ne i32
%t56, 0`.

Prepared control-flow also preserves the authoritative join intent for `%t59`:

```text
join_transfer logic.end.54 result=%t59 ... source_incomings=(logic.rhs.end.53, logic.skip.52)
  incoming [logic.rhs.end.53] -> %t57
  incoming [logic.skip.52] -> 0
parallel_copy logic.skip.52 -> logic.end.54
  move[0] 0 -> %t59 ... carrier=select_materialization
```

The first bad generated instruction sequence is in
`build/c_testsuite_aarch64_backend/src/00196.c.s` lines 147-177. The branch
sense is correct:

```asm
bl fred
mov x13, x0
cmp w13, #0
b.ne .LBB91_18
b .LBB91_19
.LBB91_19:
mov x13, #0
b .LBB91_21
```

But the join block then recomputes the RHS-only value with stale `%t55/w21`:

```asm
.LBB91_21:
add w9, w21, #1
str w9, [sp]
...
cmp w9, #0
cset w13, ne
...
mov w1, w13
bl printf
```

Owner boundary: AArch64 scalar/control publication at the short-circuit join,
not C semantics and not branch lowering. The relevant prepared block is
`logic.end.54`, where `%t56`/`%t57` are RHS-derived same-block producers and
`%t59` has authoritative out-of-ssa edge transfers. The AArch64 path around
`lower_scalar_with_address_materialization` / `lower_scalar_control_value_instruction`
materializes a same-block binary/compare for `%t57` in the join and lets that
stale publication feed the `printf` argument instead of respecting the
prepared skip-edge `%t59 = 0` publication.

Idea 366 appears causal as a trigger but adjacent in semantics: the regression
is not string-pointer comparison itself. The newly added address-materialized
scalar lowering interaction can combine a string address materialization in
the join block with scalar compare publication, exposing the older unsafe
same-block RHS materialization across a short-circuit join.

## Suggested Next

Add focused backend coverage before repair for a short-circuit join whose
skip edge publishes an immediate false value while the RHS edge computes a
same-block derived binary/compare value. The assertion should require the
generated/join publication to use the prepared edge value for the skip path and
not recompute the RHS-only binary at the join from a stale register. Include an
address materialization in the same join block to cover the idea 366 trigger.

## Watchouts

Do not repair by changing branch sense or by weakening `00196` expectations.
The branch to `logic.skip.52` is already correct. Preserve `00112`: the
delegated proof shows it still passes. Keep the fix scoped to AArch64
control/scalar publication at join edges and avoid changing unrelated
short-circuit semantics or string-pointer materialization contracts.

## Proof

Ran the delegated Step 1 localization proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00112_c)$' > test_after.log 2>&1
```

Result: failed for `c_testsuite_aarch64_backend_src_00196_c` with
`RUNTIME_MISMATCH`; passed for `c_testsuite_aarch64_backend_src_00112_c`.
`test_after.log` is the preserved proof/localization log.
