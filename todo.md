Status: Active
Source Idea Path: ideas/open/95_aarch64_calls_immediate_scalar_argument_publication_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Immediate Publication Boundary And Proof Subset

# Current Packet

## Just Finished

Activated plan Step 1: `Confirm Immediate Publication Boundary And Proof Subset`.

## Suggested Next

Execute Step 1 by auditing the current immediate scalar publication helpers
and recording the owner boundary plus exact focused proof command here before
any implementation edits.

## Watchouts

- Keep this route limited to AArch64 calls-side immediate scalar call argument
  publication.
- Do not move publication-source authority, prepared call argument plans,
  source-home selection, scalar producer selection, aggregate byval transport,
  f128 carrier handling, ordinary before-call moves, or call-boundary record
  ownership into the new owner.
- Preserve supported immediate forms, unsupported diagnostics, inline-asm
  spelling, record fields, selection status, and assembly output.
- Keep `lower_before_call_immediate_binding` as the consumer of prepared
  immediate argument facts.

## Proof

No validation run for activation-only lifecycle work; no implementation files
were touched.
