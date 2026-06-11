# Route 4 publication identity adapter

## Goal

Migrate one selected publication identity reader or wrapper boundary to Route 4
BIR records while retaining prepared publication mechanics and output
compatibility.

## Why This Exists

The Phase B2 readiness audit classifies Route 4 schema as sufficient for
current-block and block-entry publication identity. It also records that wrapper
or edge-publication consumers need a compatibility adapter because move/home
policy, storage, stack-source extension, block-order emission, wrapper
formatting, and emitted strings remain outside BIR schema.

This idea is the bounded follow-up for the Phase B2 Route 4 candidate:
selected publication identity or wrapper adapter.

## In Scope

- Select exactly one current-block, block-entry, edge-publication, or wrapper
  reader that needs publication identity.
- Use Route 4 records or indexes for block/program-point availability,
  destination/source value identity, source producer identity, producer
  instruction/index, and source-producer kind.
- Preserve prepared lookup fallback for publication mechanics, wrapper
  compatibility, and emitted/debug output.
- Prove absent, mismatch, duplicate, ambiguous, and wrong-reference behavior.
- Preserve x86/riscv wrapper or route-oracle output when affected.

## Out Of Scope

- Replacing all edge-publication lookups or public prepared publication helper
  groups.
- Moving edge-copy emission, move/home/storage policy, stack-source extension,
  block-order emission, immediate publication payload spelling, wrapper
  formatting, or emitted strings into BIR schema.
- Cleaning up retained prepared policy as a standalone task.

## Acceptance Criteria

- The selected reader obtains only Route 4 semantic publication identity from
  BIR.
- Prepared fallback remains for mechanics, target-local policy, wrappers, and
  output/oracle behavior.
- Proof covers success, absence, mismatch, duplicate/ambiguous rejection,
  wrong-reference fallback, and unchanged output where wrappers or diagnostics
  are affected.

## Reviewer Reject Signals

- Reject a slice that treats edge-copy emission, move/home/storage policy,
  stack-source extension, block-order emission, wrapper formatting, immediate
  publication payloads, prepared edge-publication lookup authority, or emitted
  strings as BIR schema.
- Reject whole `edge_publications` replacement or helper privatization.
- Reject wrapper migration without output proof and duplicate/ambiguous or
  wrong-reference coverage.
- Reject expected-string rewrites, unsupported downgrades, or route-only proof
  that hides a prepared fallback regression.
- Reject an adapter that broadens beyond one selected reader or wrapper
  boundary.

## Completion Note

Closed on 2026-06-11 after the selected AArch64 call-boundary current-block
publication source reader was migrated to consume Route 4 only for semantic
publication identity. Prepared publication lookup remains authoritative for
mechanics and output-sensitive behavior, including move/home/storage policy,
stack-source extension, block-order emission, wrapper formatting, immediate
payload spelling, emitted strings, and allowed prepared fallback.

Additional Route 4 reader migrations remain intentionally out of scope for this
bounded idea and should be opened as separate ideas if pursued.
