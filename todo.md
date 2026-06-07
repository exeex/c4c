Status: Active
Source Idea Path: ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Proof And Closure Package

# Current Packet

## Just Finished

Step 5 - Final Proof And Closure Package completed the final proof package for
idea 116 without changing implementation files, tests, build metadata,
`plan.md`, source ideas, or baseline logs.

- Migrated shared prepared facts now cover edge-copy materializable producer
  classification through
  `prepared_edge_copy_source_facts_have_materializable_producer`.
- Migrated shared prepared facts now cover edge-publication source consistency
  through `prepared_edge_publication_source_home_matches_source` and
  `prepared_edge_publication_source_memory_matches_access`; AArch64 edge-load
  dispatch fails closed when a prepared source-memory fact mismatches the load
  access.
- Migrated shared prepared facts now cover current-block join instruction
  routing through
  `prepare_current_block_join_parallel_copy_instruction_routing`, including
  prepared incoming-expression and source-result markers sized to the block's
  instruction stream.
- Migrated shared prepared facts now expose direct-global select-chain
  dependency lookup through `find_prepared_direct_global_select_chain_dependency`
  so `dispatch_producers.cpp` consumes the shared select-chain contract instead
  of carrying target-neutral traversal authority locally.
- Remaining keep-local decisions are machine-register hazards, not missing
  shared producer facts: `prepared_value_home_reads_register_index`,
  `value_publication_may_read_register_index`,
  `block_entry_move_clobbers_current_join_publication`, and
  `edge_value_publication_may_read_register_index` all depend on AArch64
  register-name parsing, general-purpose register-index comparison, alias
  policy, or call-boundary physical destination registers.
- Step 4 evidence recorded that `dispatch.cpp` and
  `dispatch_value_materialization.cpp` did not gain new producer-discovery
  logic; they continue to wire or consume prepared facts while AArch64 emission
  and register-hazard ordering remain local.
- Test/dump evidence is the focused prepared/AArch64 backend subset:
  `backend_prepared_lookup_helper` covers the prepared lookup helpers and
  current-block/edge fact contracts, and
  `backend_aarch64_instruction_dispatch` covers the AArch64 dispatch route that
  consumes those prepared facts.
- No supported-path expectations were weakened in this Step 5 package, and the
  recorded route does not rely on named-case shortcuts, predecessor rescans,
  BIR-name matching, or testcase-shaped producer discovery.
- Deferred follow-up: supervisor may choose broader regression-guard or
  plan-owner closure review. No executor-owned blocker remains in this package.

## Suggested Next

Supervisor review for closure of
`ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md`.
Executor assessment: closure-ready from this packet's owned scope, subject to
supervisor acceptance of the focused proof and any broader validation policy.

## Watchouts

- Closure should not require moving AArch64 instruction spelling, register
  allocation policy, memory operand emission, branch emission, or physical
  register hazard policy into shared prepared code.
- If later work touches the keep-local hazard helpers, preserve the boundary:
  shared prepared code may expose target-neutral producer/publication facts, but
  AArch64 code owns physical register parsing, aliasing, and ordering hazards.
- The focused proof was green, but supervisor still owns whether idea closure
  needs a broader regression guard before commit or lifecycle transition.

## Proof

Exact delegated proof command:

```sh
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1
```

Result: passed. `test_after.log` records `ninja: no work to do`, then
`backend_aarch64_instruction_dispatch` and `backend_prepared_lookup_helper`
both passed, with `100% tests passed, 0 tests failed out of 2`.

Supervisor acceptance validation:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: monotonic narrow regression guard passed with no pass/fail delta, and
the broader backend subset passed `179/179`.

Proof log path: `test_after.log` during packet proof; accepted proof is rolled
forward to `test_before.log` after supervisor regression review.
