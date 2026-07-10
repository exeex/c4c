Status: Active
Source Idea Path: ideas/open/672_stack_passed_parameter_home_dump_contract_split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh ABI And Parameter-Home Dump Evidence

# Current Packet

## Just Finished

Plan Step 1 refreshed the focused dump evidence for
`backend_dump_riscv64_stack_passed_parameter_home_publication`.

Command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_dump_riscv64_stack_passed_parameter_home_publication$') > test_after.log 2>&1
```

Result: exit `8`; build completed, the single focused dump test failed on the
expected snippet contract.

First missing snippet:

```text
abi_binding destination_kind=call_argument_abi
destination_storage=stack_slot abi_index=8 stack_offset=0
```

Current caller ABI binding facts:

- `abi_index=8` is a register argument:
  `destination_storage=register ... reg=a7`.
- `abi_index=9` is the first stack argument:
  `destination_storage=stack_slot ... stack_offset=0`.
- `abi_index=10` is a floating-point register argument:
  `destination_storage=register ... reg=fa1`.
- `abi_index=11` is a stack argument:
  `destination_storage=stack_slot ... stack_offset=8`.
- `abi_index=12` is a stack argument:
  `destination_storage=stack_slot ... stack_offset=16`.

Current callee parameter-home facts:

- `%p.fdB value_id=10` is published in stack slot `#10` at offset `40`.
- `%p.C value_id=12` is published in stack slot `#12` at offset `48`.
- `%p.fdC value_id=13` is published in stack slot `#11` at offset `44`.

Owner classification: stale snippet expectation. The current dump already
publishes the caller ABI bindings and the callee parameter homes separately;
the failure comes from expecting old stack ABI indices and old parameter value
IDs/offsets, not from missing caller stack-binding exposure or missing callee
parameter-home publication.

## Suggested Next

Proceed to Step 2 by updating only the stale expected dump snippets for
`backend_dump_riscv64_stack_passed_parameter_home_publication`, anchored to the
current ABI binding and callee home facts above.

## Watchouts

- Keep ideas 647 and 655 parked unless new evidence proves their
  stack-destination fan-in authority scope is required.
- Do not update expectations without current caller ABI and callee
  parameter-home evidence.
- Do not claim text-only expectation churn as backend capability progress.
- The current evidence supports an expectation-contract repair only; it does
  not prove new backend capability.

## Proof

Focused evidence proof:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_dump_riscv64_stack_passed_parameter_home_publication$') > test_after.log 2>&1
```

Result: exit `8`; `test_after.log` preserved for the failing dump-contract
evidence.
