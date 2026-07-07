Status: Active
Source Idea Path: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Scalar-Control-Flow RV64 Representatives

# Current Packet

## Just Finished

Step 3 - Prove Scalar-Control-Flow RV64 Representatives: created
`build/agent_state/560_step3_scalar_control_flow.allowlist` with the ten
delegated RV64 gcc torture rows and ran the delegated progress proof. The
representative subset did not pass or advance beyond the semantic admission
diagnostic: all ten rows still fail with `[RV64_C4C_OBJ_COMPILE_FAIL]` and
`scalar-control-flow semantic family`.

Per-row classification:

- `src/20000314-3.c`: still original diagnostic; latest function failure is
  `attr_eq` in `scalar-control-flow semantic family`.
- `src/20080502-1.c`: still original diagnostic; latest function failure is
  `foo` in `scalar-control-flow semantic family`.
- `src/930614-1.c`: still original diagnostic; latest function failure is
  `main` in `scalar-control-flow semantic family`.
- `src/980604-1.c`: still original diagnostic; latest function failure is
  `main` in `scalar-control-flow semantic family`.
- `src/ieee/fp-cmp-8.c`: still original diagnostic; latest function failure is
  `test_isunordered` in `scalar-control-flow semantic family`.
- `src/ieee/fp-cmp-8f.c`: still original diagnostic; latest function failure is
  `test_isunordered` in `scalar-control-flow semantic family`.
- `src/ieee/fp-cmp-8l.c`: still original diagnostic; latest function failure is
  `test_isunordered` in `scalar-control-flow semantic family`.
- `src/ieee/pr38016.c`: still original diagnostic; latest function failure is
  `test_isunordered` in `scalar-control-flow semantic family`.
- `src/pr35456.c`: still original diagnostic; latest function failure is
  `not_fabs` in `scalar-control-flow semantic family`.
- `src/pr39501.c`: still original diagnostic; latest function failure is
  `float_min1` in `scalar-control-flow semantic family`.

## Suggested Next

Delegate a follow-up implementation packet for the remaining
scalar-control-flow semantic admission failure, starting with
`src/20000314-3.c` / `attr_eq` because it remains at the original Step 3
diagnostic.

## Watchouts

- Keep function-signature work in
  `ideas/open/561_bir_function_signature_semantic_producer_admission.md`.
- Keep scalar-binop work in
  `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.
- Do not claim scalar-control-flow progress through expectation rewrites,
  unsupported downgrades, allowlist edits, or named-case shortcuts.
- The Step 3 proof command pipeline exited 0, but the progress harness summary
  reports `total=10 passed=0 failed=10`; treat the subset as not acceptance
  green.
- `src/20000314-3.c` / `attr_eq` specifically did not advance beyond
  `scalar-control-flow semantic family`.
- No expectations, unsupported markers, implementation files, tests, docs, or
  `tests/c/external` allowlists were edited in this proof packet.

## Proof

Ran `{ cmake --build --preset default && ALLOWLIST=build/agent_state/560_step3_scalar_control_flow.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh; } 2>&1 | tee test_after.log`.
Build completed with `ninja: no work to do`; the RV64 progress harness reported
`total=10 passed=0 failed=10`. Proof log: `test_after.log`. Per-case logs live
under `build/rv64_gcc_c_torture_backend/`; summary:
`build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`.
