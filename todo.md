Status: Active
Source Idea Path: ideas/open/364_aarch64_synthetic_select_label_uniqueness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Duplicate-Label Evidence

# Current Packet

## Just Finished

Step 1: refreshed duplicate-label evidence for idea 364 without touching code
or expectations. The focused AArch64 proof is green, and the generated
`build/c_testsuite_aarch64_backend/src/00143.c.s` synthetic select labels are
currently unique: 152 `.Lselect_mat_*` label definitions, 0 duplicate
definitions. The formerly colliding family appears no longer live in current
output; representative generated labels now include the extra uniqueness
component, e.g. `.Lselect_mat_1_24_163_396_9_0_true` and
`.Lselect_mat_1_24_163_396_10_0_true`.

## Suggested Next

Supervisor should decide whether idea 364 can be closed as stale evidence or
whether a plan-owner/reviewer pass should retire or replace this runbook with a
new live first-bad-fact search.

## Watchouts

- Do not continue synthetic-label repair against `00143.c.s` without a fresh
  duplicate-label reproduction; the current generated assembly has no duplicate
  `.Lselect_mat_*` definitions.
- Passing `c_testsuite_aarch64_backend_src_00143_c` is evidence that the old
  collision owner is not live for this focused case, not proof that all
  synthetic select label generation is globally covered.
- Do not reopen block-label ordering or scalar-cast source publication without
  fresh first-bad-fact evidence.
- Do not weaken expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, CTest registration, or assembler/test
  contracts.

## Proof

Ran the delegated proof exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00143_c)$' > test_after.log 2>&1`.
Build reported `ninja: no work to do`; CTest passed 2/2
(`backend_aarch64_instruction_dispatch`,
`c_testsuite_aarch64_backend_src_00143_c`). Proof log:
`test_after.log`.
