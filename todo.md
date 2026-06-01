# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue owner-by-owner relocation

## Just Finished

Completed Step 3 from `plan.md`: relocated the current-block
join/publication hazard helper family out of dispatch publication and into the
AArch64 producer owner. `block_entry_move_clobbers_current_join_publication`,
`prepared_value_home_reads_register_index`, and
`value_publication_may_read_register_index` are now declared from
`dispatch_producers.hpp` and defined in `dispatch_producers.cpp`;
`dispatch_publication.cpp` no longer defines them and `dispatch_publication.hpp`
no longer declares them directly.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `todo.md`

## Suggested Next

Classify the remaining dispatch-publication helpers owner-by-owner, starting
with `record_address_materialization_result` only if the supervisor keeps Step
3 active and wants address-materialization ownership handled next.

## Watchouts

- Call sites that need the moved declarations now include the producer owner
  directly; `dispatch_publication.hpp` does not indirectly export
  `dispatch_producers.hpp`.
- The moved current-block read hazard helper still consumes publication-owner
  state through existing public helpers (`prepared_value_home_for_value` and
  `value_has_current_block_entry_publication`); the producer owner now owns the
  same-block source traversal and register-read classification.
- `record_address_materialization_result` still lives in
  `dispatch_publication.cpp`; do not move it without an address-materialization
  ownership packet.
- Root-level `test_before.log` and `test_baseline.log` were already present
  alongside the required `test_after.log`; this packet wrote only
  `test_after.log`.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log"
```

Result: passed. The build completed and CTest reported `100% tests passed, 0
tests failed out of 169`.

Proof log: `test_after.log`
