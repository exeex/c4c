Status: Active
Source Idea Path: ideas/open/363_rv64_object_parameter_home_coverage.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Representative and Baseline Validation

# Current Packet

## Just Finished

Completed `plan.md` Step 4 representative validation for `src/20030914-2.c`.

The delegated representative harness exited nonzero. The case did not reach
the newly structured byval stack-slot parameter-home diagnostic:

```text
unsupported_byval_param_home: RV64 object route does not yet lower byval aggregate parameter homes in prepared stack slots
```

Instead, it stopped earlier during prepared module admission:

```text
unsupported_function_admission: RV64 helper-free variadic entry lowering remains unsupported without an explicit supported variadic entry runtime contract
```

This is not a passing runtime case and does not prove the Step 2 diagnostic
against the representative. It is still useful representative progress because
the recorded boundary is structured and honest: current `src/20030914-2.c`
validation is blocked before parameter-home admission by helper-free variadic
entry admission.

Files changed in this packet:

- `todo.md`

## Suggested Next

Ask the supervisor to decide whether idea 363 should close on focused backend
coverage plus this earlier representative boundary, or whether lifecycle state
needs review because the representative no longer exercises the audited
parameter-home class before helper-free variadic admission.

## Watchouts

- Treat `src/20030914-2.c` as a representative signal, not an implementation
  key.
- Do not treat the new diagnostic as support for all stack-slot formals; sret
  stack-slot formals remain a separate contract.
- Do not conflate callee byval formal homes with caller aggregate-address
  call-argument transport.
- Do not reopen scalar vararg, `va_arg`, `va_copy`, or variadic helper work
  from idea 362 and its follow-ups.
- Do not downgrade allowlists or expectations to force this case through the
  helper-free variadic entry boundary.
- If the plan remains active, the next packet needs a supervisor-selected
  representative or lifecycle adjustment that actually reaches the audited
  parameter-home class.

## Proof

Ran delegated representative proof exactly:

```sh
tmp=$(mktemp); printf 'src/20030914-2.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

Result: failed with exit code 1. `test_after.log` records one failed
representative case, and
`build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log` records the
earlier `unsupported_function_admission` boundary shown above.
