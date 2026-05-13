Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit HIR-To-LIR Handoff Compatibility Tags

# Current Packet

## Just Finished

Step 4 is complete. Lowerer local, param, static/global, generated-local,
local-const, label, local-prototype, and function-pointer signature mirrors now
either require structured identity or explicit rendered compatibility. Retained
name-only compatibility is limited to no-metadata/generated bridge cases, and
labels remain route-local block bookkeeping rather than a rendered compatibility
fallback.

## Suggested Next

Begin Step 5 by auditing HIR-to-LIR owner, type, and layout compatibility tags.
Inspect handoff callers that consume rendered compatibility metadata, then
remove or hard-fence any rendered fallback that can still override complete
structured metadata. Leave final link/output spelling intact when it is output
spelling only.

## Watchouts

- Do not weaken or rewrite expectations as HIR-to-LIR progress; Step 5 should
  preserve final output spelling while preventing rendered tags from becoming a
  second semantic authority.
- Keep retained compatibility comments concrete: owner, limitation, and removal
  condition.
- If the handoff audit finds no semantic fallback to fence, record the exact
  evidence in `todo.md` rather than adding unnecessary code churn.

## Proof

Step 5 has not run validation yet. Step 4 proof was recorded before this
lifecycle handoff.
