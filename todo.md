Status: Active
Source Idea Path: ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1.

## Suggested Next

Refresh the current `00181` first bad fact and determine whether the historical
scalar-formal post-call preservation owner is live.

## Watchouts

- The source idea is parked and says the scalar `%p.n` post-call owner was
  previously repaired; do not implement from stale `00181` evidence.
- Confirm whether current generated AArch64 reloads live scalar formals from
  their preserved homes before touching lowering code.
- Do not reopen pointer-formal callee-saved home publication, stack-preserved
  pointer formal overwrite handling, address-valued publication, semantic
  string-load work, or frontend admission inside this plan without lifecycle
  routing.

## Proof

Not run during lifecycle-only activation.
