Status: Active
Source Idea Path: ideas/open/361_rv64_vararg_abi_hook_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Materialize or Diagnose Aggregate `va_arg` Operand Homes

# Current Packet

## Just Finished

Step 4 completed the RV64 aggregate `va_arg` operand-home/access-plan packet.
`populate_variadic_entry_plans` now publishes complete RV64
`va_arg_aggregate` carriers for overflow-area aggregate payloads when prepared
destination/source homes and payload ABI are available. The implementation
reuses the shared aggregate overflow plan shape and keeps AAPCS64 HFA/register
save handling isolated to the AAPCS64 path.

Focused prepared-printer and frame-stack-call tests now assert RV64 aggregate
`va_arg` source `va_list`, aggregate destination payload, and overflow access
plans. Scalar `va_arg` and `va_copy` missing facts remain explicit in the RV64
helper-family tests for later packets. An aggregate helper call without payload
ABI now reports the narrower `target_abi.va_arg_aggregate.payload_abi`
diagnostic rather than a generic aggregate access-plan missing fact.

Representative outcomes:

- `src/20030914-2.c` still reaches `RV64 object variadic function lowering is
  not implemented`, with no helper-resource or operand-home missing facts in
  that no-helper representative case.
- `src/920908-1.c` now also reaches `RV64 object variadic function lowering is
  not implemented`, with no helper-resource, `va_start`, or aggregate
  `va_arg` operand-home/access-plan missing facts in the representative case
  log.

## Suggested Next

Start the next packet on RV64 scalar `va_arg` operand-home/access-plan
materialization. Keep it focused on scalar `va_arg` source/result homes and
overflow-area scalar access plans; leave `va_copy` for a separate packet unless
the supervisor chooses otherwise.

## Watchouts

Keep the next packet semantic and target-wide: do not make representative GCC
cases pass by expectation changes or testcase-shaped shortcuts. RV64 aggregate
`va_arg` intentionally uses overflow-area plans only; do not import AAPCS64
HFA/register-save assumptions. Scalar `va_arg` and `va_copy` missing facts
remain explicit in focused helper-family tests for later packets.

## Proof

Ran the delegated command exactly and preserved `test_after.log`:

```sh
{ cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_riscv_object_emission)$' && printf 'src/20030914-2.c\nsrc/920908-1.c\n' > /tmp/rv64-vararg-aggregate-va-arg-allowlist.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-aggregate-va-arg-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1 || true
```

Build and the four focused CTest targets passed. The two representative RV64
allowlist cases still fail as expected at the broader RV64 object-route
variadic unsupported diagnostic, with the residual outcomes recorded above.
`test_after.log` is the preserved proof log.
