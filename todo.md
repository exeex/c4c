Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace Representative Prepared Publications

# Current Packet

## Just Finished

Lifecycle review after the prior Step 5 found that idea 392 is not complete.
The representative `va-arg-13.c` still aborts with the same value-publication
boundary: the first and second `dummy` call argument objects are overwritten
with the local `va_list` storage address (`s1 == sp+0x80`) instead of the
initialized save-area pointer payload. The active runbook has been replaced
with a continuation focused on why the explicit prepared
`call_argument_value_publications` route is not owning the representative.

## Suggested Next

Execute Step 1 from `plan.md`: inspect the representative prepared output and
classify whether the explicit call-argument value-publication facts are absent,
mismatched, carrying the wrong effective payload, or present but skipped by
backend object emission. Compare directly against the focused Step 4 backend
fixture that passed.

## Watchouts

- Do not hard-code `va-arg-13.c`, `test`, `dummy`, literal registers, stack
  offsets, or the abort branch.
- Do not reopen idea 391 unless evidence shows incoming variadic GPR payloads
  are no longer saved into the backing area.
- Do not treat generic `StoreLocalPublication` records alone as callsite
  authority; Step 392 requires the explicit prepared
  `call_argument_value_publications` fact and its referenced source/destination
  records to agree.
- Keep the argument object address, the local `va_list` storage address, and
  the initialized save-area pointer payload separate in all notes and proof.

## Proof

Lifecycle-only rewrite. No build or runtime proof was run by the plan owner.
Relevant existing evidence remains in `test_after.log` and
`build/agent_state/392_step5_va-arg-13.*.log`.
