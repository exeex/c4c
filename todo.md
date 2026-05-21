Status: Active
Source Idea Path: ideas/open/354_aarch64_external_call_symbol_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The 00187 Call/Symbol-Home Boundary

# Current Packet

## Just Finished

Lifecycle switched from closed idea 348 to the new 00187 external
call/symbol-home publication idea. No implementation packet has started for
this plan.

## Suggested Next

Localize `c_testsuite_aarch64_backend_src_00187_c` from prepared homes through
generated AArch64 call setup and identify the exact call operand, symbol home,
local home, or preserved stack-home publication boundary.

## Watchouts

- Do not fold this residual back into indexed aggregate selected-address or
  selected-snapshot/writeback without fresh generated-code evidence.
- Do not narrow the route to `00187`, one callee, one symbol, one stack slot,
  or one emitted call neighborhood.
- Preserve the completed `00130`, `00176`, and `00195` guardrails from idea
  348.

## Proof

Lifecycle-only switch. Close-time focused regression guard for idea 348 passed
with `test_before.log` versus `test_after.log`: 6/8 to 7/8, no new failing
tests, and `c_testsuite_aarch64_backend_src_00176_c` resolved. Supervisor
broader backend guard before this switch passed 141/141.
