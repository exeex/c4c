# Current Packet

Status: Active
Source Idea Path: ideas/open/717_prepared_mir_join_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2.3.1
Current Step Title: Close producer and semantic-slot fail-closed gaps

## Just Finished

- Plan Step 2.4 review found three remaining fail-closed gaps: prepared-MIR
  availability does not require exactly one pointer matching the claimed
  producer kind, BIR availability does not require producer-block agreement
  with the predecessor, and duplicate identity incorrectly includes source
  rather than using semantic edge plus destination slot.
- The blocking evidence and required negative proof are recorded in
  `review/idea717_step24_authority_correction_review.md`.

## Suggested Next

- Execute Plan Step 2.3.1: enforce exactly-one kind-matching producer identity,
  producer/predecessor agreement, and semantic edge-plus-destination
  duplicate/conflict rejection across the prepared-MIR and BIR boundaries.

## Watchouts

- Do not treat differing source authority as a distinct row when predecessor,
  successor, and destination identify the same semantic join slot; it is a
  conflict that must fail closed.
- Publication, move, bundle, and producer pointers in BIR facts remain opaque
  identity tokens after the prepared core lifetime; correction proof must not
  dereference them after owner destruction.
- Do not repeat Step 2.4 or begin Step 3 until the prepared-MIR and BIR negative
  matrix covers the blocking cases.

## Proof

- Prior Step 2.3 focused proof was green but did not cover the exact-one
  producer-pointer, producer/predecessor mismatch, or conflicting semantic-slot
  cases identified by the Step 2.4 review.
- Step 2.3.1 requires a fresh supervisor-delegated build and focused
  prepared/BIR negative proof before re-review.
