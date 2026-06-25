Status: Active
Source Idea Path: ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared Frame-Slot Memory Facts

# Current Packet

## Just Finished

Lifecycle switch completed from the deferred idea 354 umbrella to child idea
368. Active execution now starts at Step 1 of the 368 runbook.

## Suggested Next

Delegate Step 1 to audit the prepared frame-slot local-memory facts for
`src/20000217-1.c`, `src/20000121-1.c`, and `src/va-arg-13.c`, then record the
first supportable RV64 address form and proof target here.

## Watchouts

- Keep idea 354 open and inactive until child ideas 368-371 close or are
  intentionally superseded.
- Do not implement aggregate `va_arg`, byval homes, or terminator lowering from
  this plan.
- Do not hard-code representative offsets or source-case names; derive address
  facts from prepared metadata.
- Put analysis logs under `build/agent_state/`, not root-level canonical logs.

## Proof

Lifecycle-only switch; no build required.
