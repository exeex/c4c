Status: Active
Source Idea Path: ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Contract And Update Consumers

# Current Packet

## Just Finished

Completed Step 3 by implementing the selected call-based placeholder identity contract in the owned prealloc files.

Implementation:

- Added `call_has_runtime_placeholder_shape(const bir::CallInst&)`, `call_is_runtime_intrinsic_placeholder(const bir::CallInst&)`, and `prepared_variadic_entry_helper_kind_for_call(const bir::CallInst&)` in `src/backend/prealloc/variadic.hpp`.
- The shared predicate requires a non-indirect call with invalid `callee_link_name_id` and no `callee_value`, then recognizes existing structured `CallInst::intrinsic`/`inline_asm` metadata or the compatibility `llvm.` placeholder token.
- Updated `src/backend/prealloc/variadic_entry_plans.cpp` so helper resource and operand-home planning classify va_start, va_copy, scalar va_arg, and aggregate va_arg through `prepared_variadic_entry_helper_kind_for_call`.
- Updated `src/backend/prealloc/call_plans.cpp::call_argument_allows_local_aggregate_address_publication` and `src/backend/prealloc/stack_layout/analysis.cpp::apply_aggregate_address_publication_hints` to reject runtime/intrinsic placeholders through `call_is_runtime_intrinsic_placeholder` instead of direct `call.callee.rfind("llvm.", 0)` checks.
- Left ordinary direct-call resolution untouched; valid `callee_link_name_id` calls do not satisfy the placeholder predicate, even if their display spelling begins with `llvm.`.

## Suggested Next

Proceed to Step 4 with focused contract proof for both affected routes: one variadic helper classification route and one aggregate-address-publication route showing runtime/intrinsic placeholders are rejected while ordinary direct-call pointer argument behavior remains unchanged.

## Watchouts

- Retained compatibility exceptions remain outside this owned slice: dynamic-stack checks in `dynamic_stack.cpp`, `frame_plan.cpp`, and `regalloc.cpp` may stay string-only for stack-state runtime compatibility; AArch64/fabs target intrinsics may continue using `CallInst::intrinsic`.
- `prepared_variadic_entry_helper_kind_for_callee(std::string_view)` remains as the raw compatibility-token parser behind the call-based wrapper; prealloc planning consumers now use the call-based classifier.
- Step 4 should guard the direct-call authority invariant explicitly: a normal linked call whose display spelling starts with `llvm.` must not be treated as a runtime placeholder.

## Proof

Delegated proof:

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: passed. Proof log: `test_after.log`.
