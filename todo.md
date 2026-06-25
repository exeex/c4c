Status: Active
Source Idea Path: ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Closure Decision

# Current Packet

## Just Finished

Completed Step 4 closure decision for the prepared-module-shape umbrella.

Decision: closure rejected/deferred. The original generated child chain through
367 is closed and the Step 3 refresh no longer shows opaque unclassified
prepared-module-shape failures, but the refresh found residual structured RV64
object-route buckets that still need lifecycle owners.

New follow-up child ideas created:

- `ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md`
- `ideas/open/369_rv64_object_route_terminator_fragment_lowering.md`
- `ideas/open/370_rv64_object_route_byval_aggregate_param_homes.md`
- `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`

Residual failing representatives from Step 3:

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

Do not close idea 354 yet. Supervisor should commit this lifecycle split, then
choose whether to deactivate/switch from the 354 umbrella to one of the new
child ideas or keep 354 active until the new children are intentionally
sequenced. Closure can be reconsidered only after 368-371 are closed or
intentionally superseded.

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

Lifecycle-only update; no build required. Step 3 representative refresh used
for the closure decision:

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
