Status: Active
Source Idea Path: ideas/open/191_bir_function_signature_byval_metadata_text_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Function Signature Byval Authority

# Current Packet

## Just Finished

Lifecycle activation created this active runbook from idea 191 and aligned the
execution pointer to Step 1.

## Suggested Next

Execute Step 1: inventory function signature byval authority in
`collect_aggregate_params`, related signature metadata helpers, and generated
signature producers. Record the text-authority site, structured metadata gap,
legacy fallback boundary, and first implementation target here.

## Watchouts

- Do not treat `signature_text` as semantic authority when structured signature
  metadata exists.
- Keep legacy no-metadata text parsing explicit rather than silently shared
  with generated metadata-rich functions.
- Do not broaden into full call ABI redesign or unrelated aggregate local-slot
  lowering.

## Proof

Lifecycle-only activation; no build or test proof required for this packet.
