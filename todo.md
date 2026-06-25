Status: Active
Source Idea Path: ideas/open/367_rv64_helper_free_variadic_entry_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Rerun the Representative Boundary

# Current Packet

## Just Finished

Completed Step 4, "Rerun the Representative Boundary."

The `src/20030914-2.c` representative no longer stops at the old helper-free
variadic admission diagnostic. It advances to the later structured
parameter-home boundary:

```text
unsupported_byval_param_home: RV64 object route does not yet lower byval aggregate parameter homes in prepared stack slots
```

This is the idea-363 byval stack-slot formal boundary, not the helper-free
variadic entry admission boundary owned by this plan.

## Suggested Next

Supervisor should route this plan to plan-owner for close/replacement decision.
The helper-free variadic admission contract is implemented and the
representative has advanced to a separate parameter-home boundary.

## Watchouts

- Do not reintroduce a requirement for `overflow_area.base_slot_id` or
  `overflow_area.base_stack_offset_bytes` on helper-free entries.
- Do not weaken helper-specific `va_start`, `va_arg`, `va_arg_aggregate`, or
  `va_copy` diagnostics; those remain separate boundaries.
- Keep the byval aggregate stack-slot parameter-home boundary out of this
  helper-free admission idea; it is a separate capability contract.

## Proof

Delegated representative proof was run exactly and exited `1`, which is
acceptable for this validation-only packet because the old helper-free
admission diagnostic is gone and the case reaches a later structured boundary.

```sh
tmp=$(mktemp); printf 'src/20030914-2.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

`test_after.log` records `total=1 passed=0 failed=1`.
`build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log` records the
current boundary:

```text
unsupported_byval_param_home: RV64 object route does not yet lower byval aggregate parameter homes in prepared stack slots
```
