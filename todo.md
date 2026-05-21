Status: Active
Source Idea Path: ideas/open/366_aarch64_string_literal_pointer_null_comparison.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair General Pointer Constant Comparison

# Current Packet

## Just Finished

Step 2 added focused AArch64 dispatch coverage for a direct string-literal
pointer/null comparison. The test now requires string address materialization
to precede `cmp x..., #0`, `cset w..., eq`, and the final I32 return
publication, proving the boolean result is defined before return.

Step 3 repaired the general direct string-pointer comparison path:
`lir_to_bir` now rewrites string-pointer aliases in pointer compare operands,
not only call arguments; stack-layout prepared addressing now publishes
pointer-value string materializations for binary operands; AArch64 dispatch can
emit exact-instruction address materializations together with scalar compare
publication; and scalar immediate operands now include pointer null immediates.
For `00112`, prepared BIR now carries `bir.eq ptr @.str0, 0`, prepared
addressing records a string materialization for `@.str0`, and generated AArch64
emits `adrp`/`add`, `cmp`, and `cset` before returning the result.

Refinement before supervisor acceptance tightened the implementation without
changing the capability: AArch64 address-materialized scalar lowering now uses
a trial scalar state and commits materialized-address/result publications only
after scalar lowering succeeds, and the LIR-to-BIR string-pointer compare
rewrite now falls back to result-name pairing when LIR and BIR pointer compare
counts differ.

## Suggested Next

Supervisor can review the pointer-constant comparison slice for commit
readiness or delegate the next active packet.

## Watchouts

The repair remains distinct from idea 356 dynamic pointer-derived byte loads:
no byte-load owner code or C test expectations were changed. The implementation
is general for direct string-pointer compare operands and pointer null
immediates, without `.str0` or register-specific handling.

## Proof

Ran the delegated proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00112_c)$' > test_after.log 2>&1
```

Result: passed. `test_after.log` reports 144/144 tests passed, including
`backend_.*` and `c_testsuite_aarch64_backend_src_00112_c`.
