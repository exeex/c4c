Status: Active
Source Idea Path: ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Implement The Agreement-Gated Adapter

# Current Packet

## Just Finished

Step 2 implemented the agreement-gated adapter around the selected
`current_block_join_prepared_query_source(...)` Route 5 reader.

Adapter boundary:

- `build_current_block_join_prepared_query_routing(...)` now passes the same
  `bir::Route5EdgeJoinSourceIndex` used for the indexed BIR identity query into
  `prepare::prepare_current_block_join_parallel_copy_source_facts(...)`.
- `current_block_join_route5_source_agrees_with_prepared(...)` now accepts a
  Route 5 source bit only when the matched prepared fact also carries
  `route5_join_source_agrees` and a concrete `route5_join_source` record.
- Non-agreement paths continue to return the existing prepared source answer:
  absent prepared inputs, missing Route 5 data, mismatched source or
  destination, wrong predecessor or successor, missing source producer,
  duplicate/conflicting Route 5 evidence, memory-source or stack-source policy
  cases, and unsupported move/source policy.
- `routing.incoming_expressions` behavior was left unchanged. Publication
  construction, parallel-copy scheduling, home/storage policy, block order,
  wrapper/output formatting, instruction selection, and emission policy remain
  outside Route 5 ownership.

## Suggested Next

Execute Step 3 from `plan.md`: prove the adapter against the selected positive
agreement path plus the existing fallback/non-agreement cases, using the same
focused subset unless the supervisor delegates broader proof.

## Watchouts

- The Step 2 change is semantic-only for one source classification reader:
  `routing.sources[instruction_index]`.
- `routing.incoming_expressions` still follows the previous Route 5/prepared
  behavior and was not part of this adapter packet.
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
