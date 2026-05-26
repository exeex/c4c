Status: Active
Source Idea Path: ideas/open/37_aarch64_branch_and_entry_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Branch And Entry Helper Ownership

# Current Packet

## Just Finished

Plan activated from
`ideas/open/37_aarch64_branch_and_entry_foldback_cleanup.md`.

## Suggested Next

Step 1 - Inventory Branch And Entry Helper Ownership: inspect
`comparison_branch_fusion.*`, `prologue_entry_formals.cpp`, include sites, call
sites, and build metadata, then record the recommended first mechanical
fold-back packet in this file without implementation edits.

## Watchouts

Keep this route mechanical. Do not change prepared branch-condition authority,
condition-code policy, branch target selection, ABI entry formal layout, byval
entry-copy semantics, F128 carrier handling, diagnostics, or tests.

## Proof

No build required for activation-only lifecycle work.
