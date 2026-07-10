Status: Active
Source Idea Path: ideas/open/672_stack_passed_parameter_home_dump_contract_split.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select Repair Owner And Patch Narrowly

# Current Packet

## Just Finished

Plan Step 2 updated only the stale `REQUIRED_SNIPPETS` contract for
`backend_dump_riscv64_stack_passed_parameter_home_publication`.

Command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_dump_riscv64_stack_passed_parameter_home_publication$') > test_after.log 2>&1
```

Result: exit `0`; build completed, and the single focused dump test passed.

Changed caller ABI binding expectation facts:

- `abi_index=8` is `destination_storage=register
  placement=gpr:call_argument#7/w1 reg=a7`, not a stack slot.
- `abi_index=9` is the first stack argument:
  `destination_storage=stack_slot ... stack_offset=0`.
- `abi_index=10` is `destination_storage=register
  placement=fpr:call_argument#1/w1 reg=fa1`, not a stack slot.
- `abi_index=11` is a stack argument:
  `destination_storage=stack_slot ... stack_offset=8`.
- `abi_index=12` is a stack argument:
  `destination_storage=stack_slot ... stack_offset=16`.

Changed callee parameter-home expectation facts:

- `%p.B value_id=9` is published in register `a7`.
- `%p.fdB value_id=10` is published in stack slot `#10` at offset `40`.
- `%p.b value_id=11` is published in register `fa1`.
- `%p.C value_id=12` is published in stack slot `#12` at offset `48`.
- `%p.fdC value_id=13` is published in stack slot `#11` at offset `44`.

This is dump-contract alignment only. It records current caller ABI bindings
and callee home facts; it is not backend capability progress and did not change
implementation code.

## Suggested Next

Proceed to Step 3 by running the supervisor-selected focused and nearby
regression subset for the stack-passed parameter-home surface.

## Watchouts

- Keep ideas 647 and 655 parked unless new evidence proves their
  stack-destination fan-in authority scope is required.
- Do not update further expectations without current caller ABI and callee
  parameter-home evidence.
- Do not claim text-only expectation churn as backend capability progress.
- The current evidence supports an expectation-contract repair only; it does
  not prove new backend capability.

## Proof

Focused repair proof:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_dump_riscv64_stack_passed_parameter_home_publication$') > test_after.log 2>&1
```

Result: exit `0`; `test_after.log` preserved for the passing focused
dump-contract proof.
