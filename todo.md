Status: Active
Source Idea Path: ideas/open/364_aarch64_synthetic_select_label_uniqueness.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative And Classify Residual

# Current Packet

## Just Finished

Step 4 proved the `00143` representative advances past the duplicate synthetic
label assembler failure after commit `a591ae012`.

Generated `build/c_testsuite_aarch64_backend/src/00143.c.s` contains 152
`.Lselect_mat_*` label definitions and zero duplicate definitions. The focused
CTest now reaches runtime and fails with `[RUNTIME_NONZERO] ... exit=1` and
empty stdout/stderr.

New runtime first bad fact:

- Source `tests/c/external/c-testsuite/src/00143.c` initializes `a[n] = n`,
  `b[n] = 0`, then uses Duff's device to copy 39 `short` elements from `a` to
  `b` before checking `a[n] == b[n]`.
- The source dispatch expression is `switch (count % 8)` with `count == 39`,
  so the expected selector is `7` and execution should enter case 7.
- Generated assembly computes `(count + 7) / 8` correctly for `n`, but the
  later `count % 8` lowering is wrong:
  `sdiv w9, w13, w9` followed by `msub w13, w9, w9, w13`.
- For `count == 39`, that sequence computes `39 - 4 * 4 == 23` rather than
  `39 - 4 * 8 == 7`.
- The switch selector therefore misses cases 0 through 7, falls through the
  default path at `.LBB1_21`, sets loop counter storage to zero, skips the
  copy, and the verification loop returns 1 because `b` remains zeroed.

Classification:

- The duplicate synthetic select/materialized-label owner from idea 364 is
  repaired for this representative.
- The remaining `exit=1` residual is an AArch64 signed integer remainder
  producer/lowering bug for `% 8`, not a synthetic-label allocation or emission
  fault.
- This residual requires lifecycle handoff rather than continued work inside
  idea 364. It is adjacent to div/rem producer-publication work, but this
  concrete fact is signed integer `%` lowering in switch selection, so it
  should not be silently folded into an unsigned-only owner without plan-owner
  review.

Closure readiness:

- Idea 364 is closure-ready from the executor perspective for the focused
  duplicate-label goal, subject to supervisor/plan-owner closure review and any
  broader validation policy they require.

## Suggested Next

Ask the plan owner to close or retire idea 364 as complete for synthetic label
uniqueness, then route the new `00143` `%` remainder residual through a separate
integer div/rem lifecycle packet.

## Watchouts

- Do not keep expanding idea 364 into runtime value correctness: the remaining
  first bad fact is arithmetic remainder lowering, not label uniqueness.
- The current proof command intentionally contains only the representative
  CTest; it does not rerun backend unit coverage from Step 3.
- No implementation files, tests, expectations, runners, `plan.md`, or
  `ideas/*` files were touched in this packet.

## Proof

Ran the delegated proof:

```sh
ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00143_c$' > test_after.log 2>&1
```

Result: command exited nonzero because
`c_testsuite_aarch64_backend_src_00143_c` now reaches runtime and returns
`exit=1`. `test_after.log` is preserved with this proof output.
