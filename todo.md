Status: Active
Source Idea Path: ideas/open/276_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Supported Stale Fixture

# Current Packet

## Just Finished

No executor packet has run for this active plan yet.

## Suggested Next

Start Step 1 - Locate The Supported Stale Fixture.

## Watchouts

- Reuse the supported stale public `memory_accesses` fixture from idea 275;
  do not use synthetic post-construction mutation as proof evidence.
- Exercise the real RISC-V same-consumer backend path, not helper-only
  formatting or classification.
- Preserve the compatible `LoadLocal` path and exact `lw a1, 12(s2)` output.
- Keep cross-publication, byte-offset coverage, x86 parity, edge-publication
  families, metadata, liveness, aggregate retirement, and draft 155 work out
  of this runbook.

## Proof

Lifecycle activation only. No build or test proof required.
