Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Current Parser Lookup Ownership

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md`.

## Suggested Next

Execute Step 1 from `plan.md`: inspect current parser lookup ownership, choose
the first narrow implementation packet, and record the packet details here.

## Watchouts

- Preserve downstream rendered-name bridge behavior.
- Use dual-read / dual-write proof before demoting any parser-owned string
  lookup path.
- Do not downgrade tests or add testcase-shaped lookup shortcuts.

## Proof

Lifecycle-only activation; no build or test proof required.
