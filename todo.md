Status: Active
Source Idea Path: ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate Target-Policy No-Change Surfaces

# Current Packet

## Just Finished

Step 4 validated the target-policy no-change surfaces for the selected
`current_block_join_prepared_query_source(...)` Route 5 reader.

Files inspected:

- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/mir/CMakeLists.txt`
- `tests/backend/bir/CMakeLists.txt`

No-change evidence:

- `build_current_block_join_prepared_query_routing(...)` still builds only the
  selected semantic source bit, accepting Route 5 source identity only after it
  agrees with the prepared current-block join source facts.
- `current_block_join_prepared_query_source(...)` remains a read of
  `routing.sources[instruction_index]`; it does not construct publications,
  move bundles, homes, storage, block order, wrappers, output text,
  instruction selection, or emission policy.
- `dispatch.cpp` still uses the selected source answer only to skip the already
  published current-block join source instruction; adjacent lowering branches
  still own move scheduling, storage/home policy, memory/address handling,
  scalar lowering, instruction construction, and final emission behavior.
- `backend_aarch64_current_block_join_routing` and
  `backend_prepared_lookup_helper` keep the prepared public helper/oracle and
  fallback rows authoritative for no-source, memory-source, duplicate/conflict,
  mismatch, missing route data, unsupported move, and adjacent prepared-owned
  facts.
- There were no expectation rewrites, baseline refreshes, helper renames,
  unsupported downgrades, facade-only changes, aggregate reshuffles, or
  named-case shortcuts in this packet.
- No ownership moved for publication construction, move/home/storage policy,
  block order, wrapper output, formatting, instruction selection, or emission
  policy.

## Suggested Next

Execute Step 5 from `plan.md`: prepare closure evidence for the narrow Route 5
current-block join-source adapter without broadening claims beyond the selected
semantic reader.

## Watchouts

- Closure evidence should stay semantic-only for one source classification
  reader: `routing.sources[instruction_index]`.
- `routing.incoming_expressions` follows the previous Route 5/prepared behavior
  and was not part of this adapter proof.
- Do not delete, privatize, rename, or hide `edge_publications`,
  `edge_publication_source_producers`, `PreparedFunctionLookups`, or
  `PreparedBirModule`.
- Do not move publication construction, move/home/storage policy, block order,
  wrapper output, formatting, instruction selection, or emission policy into
  Route 4/5 ownership.
- Do not rewrite expectations, helper names, supported-path status, wrapper
  output, or baselines as proof.

## Proof

Exact delegated proof command:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_current_block_join_routing|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log
```

Result: passed, 3/3 tests.

Proof log: `test_after.log`.
