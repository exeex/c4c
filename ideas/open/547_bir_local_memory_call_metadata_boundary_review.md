# BIR Local-Memory And Call-Metadata Boundary Review

Status: Open
Type: Producer-boundary review
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: BIR semantic producer and prepared contract

## Goal

Review current local-memory and call-metadata failures to decide which facts
must be produced by BIR/prepared layers before RV64 lowering can consume them.

## Why This Exists

The current bucket map records 44 `unsupported_local_memory_access` rows and
the source umbrella also requires call-metadata cleanup. The current evidence
does not provide a verified call-metadata row count, so this idea keeps the
work in a producer-boundary review until row-level evidence exists.

## In Scope

- Classify current `unsupported_local_memory_access` rows by first owner.
- Reconstruct current call-metadata failure rows if they exist.
- Identify missing address provenance, GEP, load/store, call argument, return,
  memcpy, or memset facts that BIR/prepared must publish.
- Create implementation ideas only after producer facts are coherent and
  first ownership is proven.

## Out Of Scope

- RV64 local-memory or call lowering that guesses missing facts.
- Treating no-count call metadata suspicions as implementation-ready.
- Reworking unrelated RV64 instruction families.
- Weakening prepared or semantic admission contracts.

## Acceptance Criteria

- The 44 local-memory rows are reviewed against current logs and assigned to
  BIR, prepared, RV64, or evidence-gap owners.
- Any call-metadata row set is reconstructed before it becomes an
  implementation queue.
- Producer-owned gaps become separate follow-up ideas before dependent RV64
  lowering proceeds.
- No tests, expectations, or unsupported markers are weakened.

## Reviewer Reject Signals

- Reject RV64 memory or call lowering that recovers missing address, argument,
  or return facts from raw target shapes.
- Reject claiming call-metadata progress without current row evidence.
- Reject helper renames, diagnostic wording changes, or expectation rewrites
  as producer capability repair.
- Reject broad BIR route rewrites unrelated to current local-memory or
  call-metadata rows.
- Reject keeping the exact old local-memory failure mode behind a new
  abstraction name.

