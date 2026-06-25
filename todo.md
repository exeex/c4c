Status: Active
Source Idea Path: ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish or Materialize the Destination va_list Address

# Current Packet

## Just Finished

Completed `plan.md` Step 3 for
`ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md`.

Changed files:

- `todo.md`

Ran the representative RV64 object-route proof for `src/920908-1.c` after the
Step 2 producer contract. The case advanced past the old destination-address
boundary:

```text
unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR home
```

The current representative stop is a later helper family that is outside this
destination-address plan:

```text
unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_arg_aggregate helper
```

## Suggested Next

Supervisor should decide whether to treat idea 366 as complete after focused
tests plus this representative advancement, or hand the active plan to the
plan owner for closure/deactivation. Any next implementation packet for the new
`va_arg_aggregate` boundary should belong to the aggregate `va_arg` backlog,
not this destination `va_list` address plan.

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen idea-365 overflow-area base-state production.
- Do not broaden into general parameter-home expansion, external variadic call
  lowering, scalar `va_arg`, `va_copy`, or aggregate `va_arg`.
- This packet intentionally rejects non-register RV64 `va_start` pointer homes
  by leaving the address fact absent rather than weakening the object consumer.
- AAPCS64 still uses the shared `materialize_va_start_destination_home()` path
  as before; this packet only changes RV64 helper operand publication.
- `src/backend/mir/riscv/codegen/object_emission.cpp` was not touched.
- Do not fix the new `va_arg_aggregate` stop inside idea 366; aggregate
  `va_arg` lowering is listed as a non-goal for this plan.

## Proof

Proof command:

```sh
tmp=$(mktemp); printf 'src/920908-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

Result: failed at the expected representative-object-route level because the
case now reaches the later aggregate `va_arg` helper boundary. `test_after.log`
records the one-case failure, and
`build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log` records the precise
diagnostic:

```text
unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_arg_aggregate helper
```
