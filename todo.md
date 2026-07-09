Status: Active
Source Idea Path: ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Residual Evidence

# Current Packet

## Just Finished

Step 1 refreshed residual stack-destination fan-in evidence for all seven
targets under `build/agent_state/647_step1_residual_fan_in/`.

- `src/20011109-2.c`: owner `rv64_prepared_move_bundle_consumer`;
  `missing_stack_destination_fan_in_authority_fact` for a select-materialized
  stack destination with register + preserved-stack + register fan-in to value
  17 at `main:block_1` before instruction 9.
- `src/20021204-1.c`: owner `rv64_prepared_move_bundle_consumer`;
  `producer_authority_missing_for_register_fan_in_stack_destination` for
  non-parallel two-register-source fan-in to stack value 18 at
  `main:tern.end.12` before instruction 1.
- `src/920429-1.c`: owner `rv64_prepared_move_bundle_consumer`;
  `producer_authority_missing_for_register_fan_in_stack_destination` for
  non-parallel two-register-source fan-in to stack value 21 at `main:entry`
  before instruction 8.
- `src/930429-1.c`: owner `rv64_prepared_move_bundle_consumer`;
  `producer_authority_missing_for_register_fan_in_stack_destination` for
  non-parallel two-register-source fan-in to stack value 14 at `main:entry`
  before instruction 6.
- `src/pr34415.c`: owner `rv64_prepared_move_bundle_consumer`;
  `producer_authority_missing_for_register_fan_in_stack_destination` for
  non-parallel two-register-source fan-in to stack value 47 at `main:entry`
  before instruction 6.
- `src/ptr-arith-1.c`: owner `rv64_prepared_move_bundle_consumer`;
  `producer_authority_missing_for_register_fan_in_stack_destination` for
  non-parallel two-register-source fan-in to stack value 13 at `main:entry`
  before instruction 8.
- `src/pr70005.c`: owner `rv64_prepared_move_bundle_consumer`;
  `producer_authority_missing_for_register_fan_in_stack_destination` for
  non-parallel two-register-source fan-in to stack value 55 at
  `fn1:logic.end.73` before instruction 2.

## Suggested Next

Use the refreshed evidence to perform Step 2 family selection. The coherent
first candidate is the six-row non-parallel two-register-source stack
destination family with missing producer authority; keep the `20011109-2.c`
select-materialized preserved-stack row separate unless the supervisor chooses
to revisit the already-proven idea 637 family.

## Watchouts

- Do not treat this as another idea 637 select-materialized semantic-merge
  packet.
- Do not infer authority from testcase identity, move-vector order, source
  order, diagnostics, final assembly, or expectations.
- Keep unrelated residual families fail-closed or split them through lifecycle
  state.
- `20011109-2.c` has the only `missing_stack_destination_fan_in_authority_fact`
  row in this set; the other six are
  `producer_authority_missing_for_register_fan_in_stack_destination` with
  `authority=none` and `parallel_copy=no`.

## Proof

Ran delegated proof:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: build completed, CTest exited nonzero with the known backend baseline
shape: 347 backend tests, 31 failures. The failed-test names in
`test_after.log` match `test_before.log`; no new backend failure rows were
introduced by this evidence-only packet.
