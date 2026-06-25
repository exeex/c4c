Status: Active
Source Idea Path: ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Closure Decision

# Current Packet

## Just Finished

Completed Step 3 representative backend evidence refresh for the
prepared-module-shape umbrella.

Representative refresh command used an 18-case allowlist spanning the original
classification buckets and later variadic residuals. Log:
`build/agent_state/354_step3_representative_refresh.log`.

Result: `total=18 passed=11 failed=7`.

Passing representatives:

- multi-block/control-flow and object-route: `src/20000113-1.c`,
  `src/20000205-1.c`
- globals/strings/data representatives: `src/20000227-1.c`,
  `src/20000314-2.c`, `src/20000223-1.c`
- mixed ABI/width/declaration/FPR/local-memory edge representatives:
  `src/20010119-1.c`, `src/20001203-1.c`, `src/20030216-1.c`,
  `src/20030330-1.c`, `src/20030125-1.c`, `src/920410-1.c`

Structured failing representatives:

- `src/20000217-1.c`: `unsupported_local_memory_access` requiring prepared
  frame-slot base-plus-offset local memory addressing.
- `src/20000224-1.c`: `unsupported_terminator_fragment`.
- `src/20000112-1.c`: `unsupported_terminator_fragment`.
- `src/20000121-1.c`: `unsupported_local_memory_access` requiring prepared
  frame-slot base-plus-offset local memory addressing.
- `src/20030914-2.c`: `unsupported_byval_param_home`.
- `src/920908-1.c`: `unsupported_variadic_helper_lowering` for
  `va_arg_aggregate`.
- `src/va-arg-13.c`: `unsupported_local_memory_access` requiring prepared
  frame-slot base-plus-offset local memory addressing.

## Suggested Next

Execute Step 4 by deciding closure vs. defer/split. Current evidence does not
show opaque unclassified prepared-shape failures, but it does show residual
structured RV64 object-route buckets without open follow-up owners.

## Watchouts

- Do not close idea 354 unless the residual structured buckets are either
  intentionally non-blocking under the umbrella acceptance criteria or are
  captured by new follow-up ideas.
- Candidate follow-up buckets from Step 3 are local memory addressing,
  unsupported terminator fragments in data/string representatives, byval
  aggregate parameter homes, and aggregate `va_arg` helper lowering.
- Do not weaken expectations or skip runtime checks.
- This is an analysis umbrella; do not make implementation or test contract
  edits from this packet.
- Put any refreshed classification logs under `build/agent_state/`, not in
  root-level canonical regression logs.

## Proof

Step 3 representative refresh:

```sh
tmp=$(mktemp); printf '%s\n' \
  src/20000113-1.c \
  src/20000205-1.c \
  src/20000217-1.c \
  src/20000224-1.c \
  src/20000227-1.c \
  src/20000314-2.c \
  src/20000112-1.c \
  src/20000121-1.c \
  src/20000223-1.c \
  src/20010119-1.c \
  src/20001203-1.c \
  src/20030216-1.c \
  src/20030330-1.c \
  src/20030125-1.c \
  src/920410-1.c \
  src/20030914-2.c \
  src/920908-1.c \
  src/va-arg-13.c > "$tmp"; \
CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/354_step3_representative_refresh.log; \
rc=$?; rm -f "$tmp"; exit $rc
```

Result: failed as expected for residual unsupported buckets,
`total=18 passed=11 failed=7`.
