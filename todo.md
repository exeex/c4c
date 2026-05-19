Status: Active
Source Idea Path: ideas/open/301_aarch64_memory_store_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm store operand failure shapes

# Current Packet

## Just Finished

Lifecycle switch complete. Umbrella idea 295 split focused idea 301 from the
committed post-300 residual inventory.

## Suggested Next

Start `plan.md` Step 1 by confirming the memory-store operand failure shapes
for `00173`, `00176`, `00181`, `00182`, `00187`, `00194`, `00213`, and
`00214`, then record the shared boundary and focused proof command here.

## Watchouts

- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not reopen closed owners 285 through 300 from failing counts alone.
- Keep runtime nonzero, runtime mismatch/crash, timeout, scalar operand-shape,
  call-boundary, unsigned reduction, invalid scalar cast spelling, and
  `lir_to_bir` residuals outside this owner unless generated-code or
  diagnostic evidence proves they share the memory-store operand failure mode.

## Proof

No tests rerun. Lifecycle-only split from the accepted post-300 backend-regex
inventory in `test_before.log`: 352 selected, 298 passed, and 54 failed.
