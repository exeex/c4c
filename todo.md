Status: Active
Source Idea Path: ideas/open/584_rv64_stack_destination_move_bundle_authority_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The Existing Authority Surface

# Current Packet

## Just Finished

Lifecycle activation created the runbook from
`ideas/open/584_rv64_stack_destination_move_bundle_authority_contract.md`.

## Suggested Next

Delegate Step 1. Map the existing prepared/prealloc move-bundle authority
surface and RV64 prepared-object consumer surface, then record the exact files,
functions, focused tests, and proof command for the taxonomy and consumer
implementation packets.

## Watchouts

- Keep idea 585 inactive; it is documentation/research and says activation is
  out of scope unless requested later.
- Do not edit implementation files in the lifecycle activation slice.
- Do not accept value-id-, filename-, function-, block-, offset-, or
  diagnostic-string-specific authority.
- Preserve fail-closed behavior for missing, unknown, unsupported, and
  genuinely ambiguous stack-destination fan-in.

## Proof

Lifecycle-only activation. No build or tests run.
