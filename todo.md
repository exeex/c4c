Status: Active
Source Idea Path: ideas/open/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build The HIR-To-LIR String Seam Inventory

# Current Packet

## Just Finished

Lifecycle activation created this execution state for `plan.md` Step 1.

## Suggested Next

Inventory the HIR-to-LIR string seams named in `plan.md` Step 1, classify each
relevant string use, identify the intended replacement authority for
identity-bearing strings, and record the first low-risk cleanup candidate with a
focused proof command.

## Watchouts

- Do not remove rendered fallback or change emitted spelling during inventory.
- Do not absorb HIR-internal legacy lookup cleanup already covered by idea 104.
- Keep printer-only, diagnostic, ABI/link spelling, and literal-byte strings
  separate from semantic identity handoffs.
- Do not downgrade tests or expectations to claim bridge cleanup progress.

## Proof

Lifecycle-only activation. No build or test command was required.
