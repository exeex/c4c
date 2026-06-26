Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture The Va List Value Boundary

# Current Packet

## Just Finished

Lifecycle switched from parked idea 391 to new active idea 392. Idea 391's
save-area route is complete in evidence but left open/parked because this
delegation forbids touching root proof logs, and the existing `test_after.log`
is intentionally the representative mismatch rather than a backend-scope close
gate log.

## Suggested Next

Execute Step 1: inspect idea 391 Step 5 logs and current prepared/BIR/object
evidence to name the initialized `va_list` payload source, the destination call
argument object, and the exact missing or ambiguous value-publication fact.

## Watchouts

- Do not solve this by hard-coding `va-arg-13.c`, `test`, `dummy`, offset
  `0x80`, offset `0x48`, or the abort branch.
- Do not reopen idea 391 unless evidence shows incoming variadic GPR payloads
  are no longer stored into the backing save area.
- The first proof target should remain focused backend/object coverage before
  the representative rerun.

## Proof

Lifecycle-only switch; no implementation validation was run and root proof logs
were not modified.
