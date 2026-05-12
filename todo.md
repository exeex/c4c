Status: Active
Source Idea Path: ideas/open/183_lir_bir_backend_freeze_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory LIR/BIR identity surfaces

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1.

## Suggested Next

Begin Step 1 by inventorying identity-bearing surfaces in `src/codegen/lir`,
`src/backend/bir`, and `src/backend/prealloc`. Record findings here before
promoting anything into `plan.md` or `ideas/open/`.

## Watchouts

- This active plan is an audit, not a backend implementation slice.
- Do not treat every retained string as a defect; classify display, diagnostic,
  output, compatibility, and route-local uses separately from semantic
  authority.
- Ideas 184-188 already exist as likely follow-up owners; only propose a new
  source idea if a blocker is genuinely uncovered.

## Proof

No validation run for lifecycle-only activation.
