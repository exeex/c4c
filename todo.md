Status: Active
Source Idea Path: ideas/open/277_phase_f5_memory_accesses_cross_publication_mismatch_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Real Consumer Fixture

# Current Packet

## Just Finished

Lifecycle activation initialized this packet from Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: locate the real `memory_accesses` consumer fixture, name the
compatible output contract, and choose the smallest supportable owner or
publication mismatch row.

## Watchouts

- Do not claim cross-publication coverage through helper-only behavior.
- Do not use synthetic mutation that normal fixture construction would reject.
- Do not weaken fallback, status/debug, route-debug, prepared-printer, wrapper,
  helper/oracle, exact-output, or baseline contracts.
- Keep byte-offset, stale-publication, x86 parity, edge-publication, metadata,
  liveness, aggregate retirement, and draft 155 work out of this plan.

## Proof

Lifecycle-only activation. No build or tests run.
