Status: Active
Source Idea Path: ideas/open/277_phase_f5_memory_accesses_cross_publication_mismatch_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Preserve The Compatible Positive Path

# Current Packet

## Just Finished

Completed Step 3 of `plan.md` by recording that the committed Step 2 proof
preserves the compatible positive path for the selected dynamic stack-source
`LoadLocal` RISC-V consumer.

The positive control still drives
`consume_edge_publication_move_intent(..., &route5_memory_edge)` through the
same selected Route 3 / Route 5 memory-source authority, asserts
`EdgePublicationMoveIntentStatus::Available`, emits exactly `lw a1, 12(s2)`,
and keeps both `route5_edge_source_agrees` and
`route3_source_memory_agrees` true.

## Suggested Next

Execute Step 4 acceptance proof with the supervisor-selected build and targeted
CTest command for `backend_riscv_prepared_edge_publication`.

## Watchouts

- The new negative proof is intentionally distinct from byte-offset drift and
  stale-offset publication: both public rows use offset `12`, and the mismatch
  is the public publication block label (`left` versus `join`).
- The fail-closed evidence depends on normal lookup construction preserving two
  public rows by source value id, not synthetic map mutation.
- Step 3 is todo-only status recording; the positive-path assertion already
  lives in the committed Step 2 test proof.
- Do not weaken fallback, status/debug, route-debug, prepared-printer, wrapper,
  helper/oracle, exact-output, or baseline contracts.
- Keep byte-offset, stale-publication, x86 parity, edge-publication, metadata,
  liveness, aggregate retirement, and draft 155 work out of this plan.

## Proof

Todo-only packet. No build or tests were run, and logs were not touched.
