Status: Active
Source Idea Path: ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prepare Closure Evidence

# Current Packet

## Just Finished

Step 5 prepared closure-ready evidence for the completed Route 5
current-block join-source identity adapter.

Selected reader:

- `current_block_join_prepared_query_source(...)`, backed by
  `build_current_block_join_prepared_query_routing(...)` in
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.
- The selected semantic read is `routing.sources[instruction_index]` for the
  Route 5 current-block join source classification.

Adapter boundary:

- The adapter accepts the Route 5 source identity only after it agrees with the
  prepared current-block join source facts.
- The adapter remains semantic-only. Publication construction, parallel-copy
  scheduling, homes/storage, block order, wrappers, output spelling,
  formatting, instruction selection, and final emission remain on the existing
  prepared/target-owned paths.

Fallback and compatibility evidence:

- Focused tests cover selected positive agreement plus no-source,
  memory-source, duplicate/conflict, mismatch, missing route data,
  unsupported-move, and adjacent prepared-owned facts.
- `backend_prepared_lookup_helper` keeps the public prepared helper/oracle
  compatibility path authoritative for fallback and public lookup delivery.
- `backend_aarch64_instruction_dispatch` remains green with the adapter in
  place, confirming instruction-dispatch compatibility for the selected
  current-block join-source boundary.
- No expectations, baselines, helper names, supported-path statuses, wrapper
  output, or public prepared APIs were changed for this closure packet.

Target-policy no-change evidence:

- `current_block_join_prepared_query_source(...)` still reads only
  `routing.sources[instruction_index]`.
- Adjacent lowering still owns move scheduling, storage/home policy,
  memory/address handling, scalar lowering, instruction construction, wrapper
  output, formatting, and emission behavior.
- No whole `edge_publications`, `edge_publication_source_producers`,
  `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155 retirement
  readiness is claimed.

## Suggested Next

Ask the plan owner to evaluate lifecycle closure for idea 241 using the narrow
Route 5 current-block join-source adapter evidence and the canonical proof logs.

## Watchouts

- Closure should stay semantic-only for one source classification reader:
  `routing.sources[instruction_index]`.
- `routing.incoming_expressions` follows the previous Route 5/prepared behavior
  and was not part of this adapter proof.
- Do not delete, privatize, rename, or hide `edge_publications`,
  `edge_publication_source_producers`, `PreparedFunctionLookups`, or
  `PreparedBirModule`.
- Do not claim whole `edge_publications`,
  `edge_publication_source_producers`, `PreparedFunctionLookups`,
  `PreparedBirModule`, or draft 155 retirement readiness.
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

Result: passed, 3/3 tests:

- `backend_aarch64_instruction_dispatch`
- `backend_aarch64_current_block_join_routing`
- `backend_prepared_lookup_helper`

Proof log: `test_after.log`.
