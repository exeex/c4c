# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 9.1
Current Step Title: Choose the BIR-owned D5 parallel-copy realization route

## Just Finished

- Plan Step 9.1's exact-current product repair is complete: after D5 copy
  resolution advances the revision, the enclosing
  `CopyResolutionTransaction` now invokes the sole constraint projection
  authority, E1 recomputation, E2's non-reallocating assignment validator,
  E3's non-mutating spill-state validator, and the existing target
  realizability registry/checker in deterministic dependency order.
- All five resolved-revision products are staged and installed atomically;
  predecessor products stay immutable, stable IDs and preservation records
  cannot rekey them, and any owner failure produces no E4 input.

## Suggested Next

- Execute Plan Step 9.2's frame-aware one-record realizability and verifier-
  interval repair without reopening the now-closed exact-current product
  ownership route.

## Watchouts

- Step 9.2 still must add a BIR-owned post-allocation/frame-aware realizability
  closure or schema restriction proving stack, call, spill, reload, and scratch
  nodes map one record before E4; this packet's exact-key target-product refresh
  deliberately does not solve that blocker.
- Step 9.2 must also separate the initial-D5 Pseudo publication verifier
  interval from the assigned E3-retry/D5-resolved private-candidate interval.
- Step 11 still must inventory current nested legacy paths and correct address
  ownership from C5 to C6.
- After those repairs, repeat Step 12 and obtain a new independent Step 13
  review. Step 14 is forbidden until that review reports zero blockers.

## Proof

- Passed the supervisor-selected documentation proof: `git diff --check`; the
  required positive `rg` over the nine owned BIR architecture contracts; and
  the required negative `rg` rejecting preservation-record minting, stable-ID
  rekeying/freshness, and accepted predecessor-keyed E1/E2/E3/realizability
  products.
- No build or test subset applies to this docs-only packet. Per the delegated
  boundary, regression logs were not created or modified.
