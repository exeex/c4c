Status: Active
Source Idea Path: ideas/open/344_semantic_bir_loop_carried_pointer_deref_provenance.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Loop-Carried Pointer Dereference Boundary

# Current Packet

## Just Finished

Lifecycle transition closed idea 343 and activated idea 344.

## Suggested Next

Begin Step 1 localization for semantic BIR loop-carried pointer dereference
provenance.

## Watchouts

- Do not reopen fixed-offset fallthrough copy emission unless fresh evidence
  again shows generated AArch64 skips consecutive prepared short-copy offsets.
- Preserve idea 342's repaired latch behavior; this packet reconfirmed the
  latch branches on the single post-decrement counter value.
- Do not special-case `00143`, `%lv.from`, `%lv.to`, `%lv.a.0`, `%lv.b.0`,
  labels, block numbers, stack offsets, source lines, emitted instruction
  strings, expectations, unsupported lists, runner, timeout, CTest, or
  proof-log policy.

## Proof

No executor proof has run for idea 344 yet.
