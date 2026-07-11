# Current Packet

Status: Active
Source Idea Path: ideas/open/718_transitive_current_block_incoming_expression_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract and register one focused probe per seam

## Just Finished

- Completed plan Step 1 without implementation or expectation changes. Canonical
  `test_before.log` records the consumer-only baseline as 317/320 passing, with
  `backend_aarch64_current_block_fixture_policy_attachment`,
  `backend_aarch64_current_block_join_routing`, and
  `backend_aarch64_instruction_dispatch` failing. The preparation evidence
  remains the same collision: the edge publication preserves `%source`, while
  incoming-expression routing also needs its producer dependency `%operand`;
  rewriting the publication source to `%operand` is not authority.
- Mapped each blocked fact to exactly one upstream seam:
  - **Direct publication identity:** `PreparedEdgePublication` production and
    `prepare_block_entry_parallel_copy_edge_source_facts` own the preserved
    source ID/name/home. The collision evidence is that the direct routed-
    operand query accepts the published `%source` and rejects `%operand`.
  - **Producer dependency edges and closure:**
    `prepare_current_block_join_parallel_copy_source_facts` currently discovers
    producer results and walks binary/cast/select operands with a processed-name
    termination set. The collision evidence is that `%operand` is reachable as
    a dependency of `%source` but is not the direct publication identity; this
    seam must own missing, cyclic, ambiguous, and conflicting closure.
  - **No-policy and zero-fact state:** the prepared parallel-copy fact result
    and the `PreparedFunctionLookups::current_block_join_routing_facts` boundary
    distinguish owner attachment from applicable routing facts. The fixture-
    policy failure observes an attached owner with no policy and zero applicable
    facts, so this seam—not owner presence—must define supported zero-fact
    semantics.
  - **Stable-key owner-fact composition:** `make_prepared_function_lookups`
    composes prepared routing facts into the function owner, and
    `query_prepared_current_block_join_routing_consumption` consumes them by
    successor plus routed value ID/name plus role while checking invariant edge
    identity/origin. The integration failures observe that the current composed
    owner facts do not yet represent both preserved `%source` and authorized
    `%operand` without collision.
- Confirmed `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`
  is unchanged and remains integration-only; no supported vector was altered
  and no capability progress is claimed by this inventory.

## Suggested Next

- Execute plan Step 2 by extracting and registering four focused backend probes,
  one for each inventoried seam, while leaving the AArch64 integration fixture
  unchanged.

## Watchouts

- Keep `backend_aarch64_current_block_join_routing` integration-only. Do not
  change supported vectors, rewrite `%source` identity to `%operand`, treat an
  attached owner as authority, or restore Route 5/target-local reconstruction.
- Fresh proof is 320/320 at current `HEAD`, unlike the frozen canonical 317/320
  blocked-family baseline. This Step 1 packet changed only lifecycle scratch
  state, so the green rerun is recorded as observation, not capability progress.

## Proof

- Exact supervisor-selected command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
- Build succeeded and the fresh matching backend subset passed 320/320; proof
  output is preserved in `test_after.log`. Canonical blocked evidence remains
  the 317/320 result in `test_before.log` named above.
