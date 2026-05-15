Status: Active
Source Idea Path: ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Helper Marshaling And ABI Binding Gap

# Current Packet

## Just Finished

Runbook exhaustion review completed: idea 236 is parked on the remaining
structured helper marshaling and ABI register-binding blocker, and idea 249 is
now active as the prerequisite runbook.

## Suggested Next

Execute Step 1 from `plan.md`: inspect selected i128 div/rem helper-boundary
records, current printer diagnostics, ABI argument/result register binding
needs, marshaling moves, call-clobber/live preservation facts, and selected
call operand ownership before code changes.

## Watchouts

- Do not close idea 236 until div/rem helper-boundary terminal call printing
  can consume structured marshaling and ABI register-binding facts, or until a
  later plan-owner explicitly narrows the source idea.
- Do not hard-code helper ABI registers, fixed `x0`/`x1` lane ownership,
  register adjacency, or clobber policy in AArch64 printer/dispatch code.
- Float/i128 conversion helper mapping and memory-return helper families
  remain outside this prerequisite.
- Do not lower helper-required i128 operations as scalar i64 shortcuts.

## Proof

Lifecycle-only split/switch. No build or test proof was required, and proof
logs were not modified.
