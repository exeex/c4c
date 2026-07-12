# Current Packet

Status: Active
Source Idea Path: ideas/open/722_direct_edge_publication_available_move_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize the unavailable direct-edge move fact

## Just Finished

- Plan Step 1 localized the first unavailable typed move fact to the out-of-SSA `PreparedMoveResolution`, before x86 emission or Route 5 compatibility. `force_supported_shared_edge_publication_homes` changes `merge`'s `PreparedValueHome` to register `ebx`, but does not regenerate or coherently update the matching block-entry move. The move therefore retains its originally prepared non-register destination storage and absent `destination_register_name`.
- `prepare_current_block_join_parallel_copy_source_facts` copies destination spelling/storage from that move. `PreparedMirFunctionView::current_block_direct_edge_publication_sources` then reaches the first blocking typed-boundary check: an otherwise available source is classified `UnsupportedDestinationHome` when the destination home is Register but the move supplies no destination register name. Consequently `consume_edge_publication_move_intent` has no `Available` source row and x86 correctly emits no move.
- The expected invariant is one coherent producer-owned tuple: the publication's destination value/home, the out-of-SSA move's `to_value_id`, `destination_storage_kind == Register`, and `destination_register_name`, plus the typed source home/freshness authority, must all describe the same edge move. A home-only post-prepare mutation is not sufficient authority.

## Suggested Next

- Plan Step 2 should repair producer preparation so a supported register destination publishes a coherent move/home/publication tuple, then prove the typed query returns `Available`; do not teach the consumer to infer a register from the home or BIR.

## Watchouts

- Supported comparison shapes already exist: `backend_x86_prepared_decoded_home_storage_test.cpp` proves a stack-source publication with coherent register destination (`ebx`) becomes an `Available` x86 move intent; `backend_prepared_mir_core_comparator_test.cpp` proves a coherent register-source/register-destination row (`r10d` to `r12d`) becomes an `Available` typed view; `backend_prepared_lookup_helper_test.cpp` covers coherent named-register, immediate, and stack source facts together.
- Focused positive proof should cover the semantic joined-branch fixture without post-prepare home-only drift and assert both typed query `Available` and emitted register move. Nearby immediate and stack rows should remain `Available`. Focused negative proof should independently drift destination storage kind, remove destination register spelling, or mismatch `to_value_id`, and assert fail-closed `UnsupportedDestinationHome`/missing publication without consumer fallback.
- The blocked idea 708 consumer diff remains parked and is not progress for this producer plan.

## Proof

- Diagnostic-only localization used AST-backed definition lookup plus narrow inspection of the typed query, producer fact builder, failing fixture, and existing positive comparison tests. The existing `backend_x86_handoff_boundary` failure remains the reproduction surface; canonical logs were not rewritten.
