Status: Active
Source Idea Path: ideas/open/154_bir_memory_access_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate Prepared Memory Surface And Evidence Set

# Current Packet

## Just Finished

Activation created the active runbook from
`ideas/open/154_bir_memory_access_identity.md`.

## Suggested Next

Execute Step 1 of `plan.md`: locate the prepared memory/access query surface,
direct callers, representative equivalence cases, rejected target/layout
fields, and the first narrow proof subset before changing implementation code.

## Watchouts

- Keep `review/phase_a_steps_1_4_route_review.md` untouched.
- Keep prepared memory/access queries available as the oracle until BIR
  equivalence is proven.
- Use BIR slot/name identity for locals instead of prepared frame slot ids.
- Do not import frame layout, concrete offsets, TLS relocation spelling,
  GOT/direct/page-low policy, base-plus-offset legality, target
  addressing-mode choice, or AArch64 memory operand legality into BIR memory
  identity.

## Proof

Activation-only lifecycle update; no build or backend tests required.
