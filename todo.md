Status: Active
Source Idea Path: ideas/open/516_rv64_multi_source_prepared_move_bundle_classification.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Classification Or Producer-Owned Rejection

# Current Packet

## Just Finished

Step 3 - Repair Classification Or Producer-Owned Rejection implemented the
minimal fail-closed classifier boundary for ambiguous non-parallel
multi-register-source stack-destination prepared move bundles.

Handled boundary:

- `classify_prepared_object_move_bundle_consumer` now accepts optional
  prepared value-home lookups and rejects a before-instruction,
  non-parallel `authority=none` move bundle when two moves have explicit
  register source homes and target the same stack destination value or stack
  slot home.
- The new shared classifier status/category is
  `ambiguous_non_parallel_multi_source_stack_destination`, with diagnostic
  text:
  `prepared move-bundle classifier rejected ambiguous non-parallel multi-source stack-destination authority`.
- RV64 object emission now passes its existing value-home lookups into the
  classifier before `fragment_for_prepared_move_bundle`, so the ambiguous
  `tests/c/external/gcc_torture/src/20010518-1.c` shape stops at the
  producer/classifier diagnostic instead of the old RV64-local
  `unsupported_move_bundle_target_shape` fragment diagnostic.

Stable behavior:

- Single register-source to stack-destination prepared move bundles still
  classify as available at the prepared-object consumer boundary.
- Existing out-of-SSA parallel-copy placement and authority classifications
  remain on their prior statuses.
- Coherent stack-to-stack and other RV64-local unsupported target shapes remain
  rejected by their existing RV64 predicates.

## Suggested Next

Execute Step 4 by adding any remaining focused coverage the supervisor wants
around adjacent unsupported shapes. The current slice already covers the main
classifier reject path and single-move stability; broader adjacent bank, width,
and sequencing variants can be split only if the supervisor wants more than
the Step 3 minimal boundary.

## Watchouts

Remaining unsupported shapes are intentionally fail-closed rather than
materialized: missing value-home lookup facts stay outside the new semantic
classification and continue to fall through to existing consumer/RV64
diagnostics; multi-move same-destination bundles without explicit parallel-copy
authority remain unsupported; unsupported banks, widths, missing source or
destination sizes, and incoherent register identities remain RV64-local
materialization rejects unless a later producer contract publishes stronger
authority.

## Proof

Proof completed for this code-changing packet:

- `cmake --build --preset default` passed.
- `build/tests/backend/bir/backend_prepared_object_consumer_contract_test`
  passed.
- `build/tests/backend/mir/backend_riscv_object_emission_test` passed.
- Focused representative command:
  `build/c4cll -I tests/c/external/gcc_torture --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20010518-1.c -o build/agent_state/20010518-1.o`
  now rejects with
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  before RV64 move-bundle fragment materialization.
- Delegated proof command
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed with final output captured in `test_after.log`.
- `git diff --check` passed for touched files.
