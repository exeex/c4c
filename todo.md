Status: Active
Source Idea Path: ideas/open/364_aarch64_synthetic_select_label_uniqueness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair General Label Uniqueness

# Current Packet

## Just Finished

Steps 2 and 3 are implemented for AArch64 synthetic select/materialized-label
uniqueness.

Focused coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`:
`repeated_select_chain_materializations_use_unique_synthetic_labels` builds one
block with two call-argument select-chain materializations under the same call
instruction, then scans emitted `.Lselect_mat_*` label definitions and fails if
any definition repeats. The fixture intentionally forces the materialized
select-chain path rather than the ordinary `csel` select lowering path.

The repair is in `src/backend/mir/aarch64/codegen/dispatch.cpp`.
`select_chain_label` now includes both the root value id and target register
index in addition to function id, block label, root instruction index, recursive
label index, and suffix. Root value id alone was not sufficient for `00143`:
the generated `00143.c.s` still had repeated materializations for the same root
value and root instruction, but into different temporary target registers. The
added target-register discriminator keeps adjacent same-root materialization
regions unique without changing `.LBB*` block-label ordering or epilogue logic.

`00143` no longer fails in assembly with duplicate `.Lselect_mat_*` symbols.
Generated `build/c_testsuite_aarch64_backend/src/00143.c.s` now contains 152
`.Lselect_mat_*` label definitions with zero duplicates. The delegated proof
advanced `00143` to a new residual: runtime nonzero, `exit=1`, with empty
stdout/stderr.

## Suggested Next

Classify the new `00143` runtime `exit=1` residual if the supervisor keeps this
plan active. The duplicate synthetic-label owner is repaired and covered.

## Watchouts

The proof command exits nonzero because `00143` now reaches runtime and returns
1. Backend tests in the delegated subset are green. This packet did not touch
`plan.md`, `ideas/open/*`, `ideas/closed/*`, expectations, unsupported
classifications, runners, block-label ordering, or epilogue code.

## Proof

Ran the delegated proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00143_c)$' > test_after.log 2>&1
```

Result: build succeeded and `backend_.*` passed. The overall command exited
nonzero because `c_testsuite_aarch64_backend_src_00143_c` advanced to
`[RUNTIME_NONZERO] ... exit=1`. `test_after.log` is preserved with this proof
output.
