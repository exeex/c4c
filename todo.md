Status: Active
Source Idea Path: ideas/open/244_aarch64_variadic_prepared_storage_and_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Attach Register-Save And Overflow Storage Authority

# Current Packet

## Just Finished

Lifecycle split completed: idea 243 is parked on a prepared-authority blocker,
and idea 244 is now active as the prerequisite runbook.

## Suggested Next

Execute Step 1 from `plan.md`: attach register-save-area and overflow-area
storage authority by populating slot id and stack offset facts, including
missing-fact diagnostics for incomplete overflow base storage.

## Watchouts

- Do not implement selected `va_start`, `va_arg`, aggregate `va_arg`, or
  `va_copy` machine-node lowering in this prerequisite.
- Do not infer AAPCS64 frame placement, overflow offsets, `va_list` layout,
  named argument counts, or scratch policy inside AArch64 target lowering.
- Preserve fail-closed diagnostics; expectation-only or diagnostic-only
  changes are not capability progress.

## Proof

Lifecycle-only split/reset. No build or test proof was required, and canonical
regression logs were not modified.
