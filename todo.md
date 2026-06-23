Status: Active
Source Idea Path: ideas/open/313_rv64_external_empty_stub_policy_runtime.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe Full External Candidate Bucket

# Current Packet

## Just Finished

Step 5 full external candidate bucket probe for idea 313.

Probed all 59 source-idea candidate cases under
`build/rv64_c_testsuite_probe_latest/triage_313_step5/` after the Step 3/4
policy repairs. The probe table is:

- `build/rv64_c_testsuite_probe_latest/triage_313_step5/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_313_step5/summary.md`

Results: `src/00025.c` passed emit, clang/link, and qemu through the preserved
`strlen` runtime-link path. The other 58 candidates stopped during emit with
`unsupported_external_call` diagnostics before runnable assembly. Required
representatives `src/00056.c`, `src/00125.c`, `src/00179.c`, and `src/00220.c`
all stopped at the unsupported variadic `printf` boundary. Additional policy
signals include `src/00174.c` stopping at fixed-arity math symbol `sin`,
`src/00187.c` at `fclose`, and `src/00207.c` at `llvm.stackrestore`, all as
unsupported runtime external symbols.

## Suggested Next

Ask the plan owner to decide closure for idea 313. Executor evidence suggests
the active runbook is closure-ready for the current narrow policy: bodyless
external stubs are gone for the candidate bucket, `strlen` remains supported,
and unsupported external surfaces now stop with explicit diagnostics. If the
plan owner wants broader runtime support, that should be a follow-up plan with
per-symbol-family link/runtime proof rather than a continuation of this probe
packet.

## Watchouts

- Do not add fake named libc/libm/string/user external bodies.
- Do not classify every external call as unsupported; `strlen` fixed-arity
  runtime linkage remains supported for the current scalar-GPR RV64 path.
- Do not allow arbitrary scalar-GPR fixed-arity externs through as runnable
  assembly without an explicit supported-runtime policy entry and proof.
- `src/00220.c` stops at its first unsupported `printf`; later library/math
  calls in that source are not reached by this first-failure probe.
- Broader libc/libm/string support remains unimplemented by policy, not a
  backend residual in this runbook.
- Existing unrelated backend proof failure:
  `backend_riscv_prepared_edge_publication`.

## Proof

Ran:

- `cmake --build --preset default -j`
- emit/link/qemu probe for all 59 source-idea candidate cases into
  `build/rv64_c_testsuite_probe_latest/triage_313_step5/probe_results.tsv`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Results:

- Build passed.
- Probe completed: `1 pass`, `58 unsupported-diagnostic`, `0
  supported-linked failure`, `0 separate backend/runtime mechanism`.
- Delegated backend proof wrote `test_after.log` and failed with the existing
  `backend_riscv_prepared_edge_publication` failure as the only failing
  backend test.
