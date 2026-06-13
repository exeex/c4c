Status: Active
Source Idea Path: ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Fallback And Public Compatibility

# Current Packet

## Just Finished

Step 3 proved fallback and public compatibility for the selected
`current_block_join_prepared_query_source(...)` Route 5 reader. Existing focused
tests were explicit enough, so no new tests were added.

Coverage accepted:

- `backend_aarch64_current_block_join_routing` covers the selected positive
  agreement path where the indexed Route 5 source identity agrees with the
  prepared current-block join source fact and sets `routing.sources`.
- The same routing test covers relevant fallback/non-agreement behavior:
  missing prepared policy, missing predecessor, mismatched source, no-source
  Route 5 evidence, duplicate/conflicting Route 5 evidence, memory-source policy
  evidence, absent Route 5 identity, and adjacent non-source producer rows.
- `backend_prepared_lookup_helper` covers public helper/oracle compatibility
  for `prepare_current_block_join_parallel_copy_source_facts(...)`, including
  agreeing Route 5 evidence, prepared-only rows, empty Route 5 index,
  no-source, source mismatch, destination mismatch, memory-source evidence,
  duplicate evidence, wrong predecessor/successor, and nearby immediate,
  stack-source, unsupported move, and adjacent prepared-owned facts.
- `backend_aarch64_instruction_dispatch` keeps the dispatch-level same-feature
  current-block join routing path in the focused subset, alongside nearby Route
  4/5 edge-publication and join-source consumers.

## Suggested Next

Execute Step 4 from `plan.md`: validate target-policy no-change surfaces for
the selected Route 5 current-block join-source adapter using the same focused
subset unless the supervisor delegates broader proof.

## Watchouts

- The adapter proof is semantic-only for one source classification reader:
  `routing.sources[instruction_index]`.
- `routing.incoming_expressions` still follows the previous Route 5/prepared
  behavior and was not part of this adapter proof.
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
