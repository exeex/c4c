Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Contract Surface Audit
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed Plan Step 1 "Contract Surface Audit" for idea 88 by auditing the
published prepared `frame_plan`, `dynamic_stack_plan`, `call_plans`, and
`storage_plans` against the current x86 contract surface.

Current audit result:
- `frame_plan` already publishes scalar fixed-frame ownership needed for target
  consumers: frame size/alignment, saved callee registers, frame-slot order,
  dynamic-stack presence, and whether fixed slots must anchor off `fp` instead
  of transient `rsp`.
- `dynamic_stack_plan` already publishes scalar stack-state ownership needed by
  x86: ordered `stacksave` / `dynamic_alloca` / `stackrestore` operations plus
  whether fixed slots must stay `fp`-anchored while dynamic stack motion is
  live.
- `call_plans` already publish scalar argument/result move destinations,
  storage homes, and call-clobber sets, and the prepared printer exposes those
  facts in a reviewable dump.
- The first concrete scalar contract gap is still in `PreparedCallPlan`: x86
  consumers that care about call-wrapper shape must still reconstruct whether a
  call is same-module, direct fixed-arity extern, direct variadic extern, or
  indirect from semantic-module details because the prepared call contract only
  publishes `is_indirect` plus optional `direct_callee_name`.

## Suggested Next

Take the first bounded Step 3 packet on the call surface: extend
`PreparedCallPlan` to publish an explicit scalar call-wrapper kind
(`same_module`, `direct_extern_fixed_arity`, `direct_extern_variadic`,
`indirect`), print it in prepared dumps, and tighten the backend/prealloc tests
around that published authority so x86 route selection no longer has to
reclassify wrapper shape from semantic-module state.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Do not expand this runbook into target-specific instruction spelling or
  generic x86 behavior recovery; the x86 module emitter is still intentionally
  stubbed, so this packet stayed at the contract boundary only.
- The prepared call gap above is scalar and bounded. It does not require
  grouped-register span/storage authority.
- A likely follow-on after wrapper-kind publication is explicit variadic call
  scalar metadata such as `%al` floating-argument count, but the first missing
  prepared fact is wrapper-kind classification itself.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$'`.
Result: pass.
Proof log: `test_after.log`.
