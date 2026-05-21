Status: Active
Source Idea Path: ideas/open/366_aarch64_string_literal_pointer_null_comparison.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative And Classify Residual

# Current Packet

## Just Finished

Step 4 proved the representative `00112` case on implementation baseline
`ef4f45482`.

Generated AArch64 in
`build/c_testsuite_aarch64_backend/src/00112.c.s` now advances past the stale
pointer-comparison return. The first block is:

```asm
main:
    adrp x20, .str0
    add x20, x20, :lo12:.str0
    cmp x20, #0
    cset w13, eq
    mov x0, x13
    ret
```

This proves the direct string-literal pointer operand is materialized before
the null comparison, and the boolean compare result is published before return.
The previous first bad fact, `mov x0, x13` without any defining
materialization/`cmp`/`cset`, is gone.

Residual classification: no new first bad fact remains for `00112` in the
representative proof. The test passes.

## Suggested Next

Supervisor can accept Step 4 for idea 366 and decide whether to close the idea
or run any broader regression guard required by lifecycle policy.

## Watchouts

Step 4 touched only `todo.md` and `test_after.log`. The proof is representative
only for `00112`; broader acceptance remains supervisor-owned.

## Proof

Ran the delegated Step 4 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00112_c$' > test_after.log 2>&1
```

Result: passed. `test_after.log` reports 1/1 test passed for
`c_testsuite_aarch64_backend_src_00112_c`.
