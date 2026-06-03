Status: Active
Source Idea Path: ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Choose The Placeholder Identity Contract

# Current Packet

## Just Finished

Completed Step 2 by selecting the smallest explicit runtime/intrinsic placeholder identity contract for the Step 1 consumers.

Selected contract shape:

- Add a shared BIR/prealloc query contract over `bir::CallInst`, not a broad new ordinary-call resolution path. The first implementation should expose named helper predicates/classifiers such as `runtime_placeholder_kind_for_call(const bir::CallInst&)`, `prepared_variadic_entry_helper_kind_for_call(const bir::CallInst&)`, and `call_is_runtime_intrinsic_placeholder(const bir::CallInst&)` or equivalent local names.
- The query authority is a narrow combination of existing BIR placeholder facts: non-indirect call, invalid `callee_link_name_id`, no `callee_value`, and an explicitly recognized runtime/intrinsic placeholder token or existing structured `CallInst::intrinsic`/`inline_asm` metadata where applicable. The raw `callee` spelling may remain the compatibility token parsed by the shared helper, but prealloc consumers should no longer scatter direct string checks as their own semantic authority.
- Variadic helper identity is the first-class structured consumer. `prepared_variadic_entry_helper_kind_for_callee(std::string_view)` should either be replaced by or wrapped behind `prepared_variadic_entry_helper_kind_for_call(const bir::CallInst&)`; `variadic_entry_plans.cpp` should consume the call-based classifier for va_start, va_copy, scalar va_arg, and aggregate va_arg routes.
- Local aggregate address publication should use the same placeholder query, not a raw `callee.rfind("llvm.", 0)` check. Runtime/intrinsic placeholders should reject local aggregate address publication through the shared predicate while ordinary direct calls still use direct-call resolution and argument ABI facts.
- Ordinary direct calls keep `callee_link_name_id` as their semantic authority. `resolve_direct_callee` must continue to prefer valid link ids, reject mismatched raw spellings, and use raw `callee` only for no-id compatibility. Step 3 must not make linked direct calls consult placeholder/raw LLVM-name logic.

Alternatives rejected:

- New all-purpose `CallInst` enum for every runtime, intrinsic, inline-asm, stack, and target intrinsic kind: too broad for the audited Step 1 consumers and likely to pull in AArch64/fabs cleanup outside this source idea.
- Merely renaming string helpers while leaving each prealloc consumer to check `call.callee` independently: this preserves raw placeholder strings as undocumented semantic authority and fails the source idea.
- Treating placeholder names as ordinary direct callees through `resolve_direct_callee`: this would blur runtime placeholders with user/extern symbol identity and risks direct-call link-name regressions.
- Widening the first implementation to dynamic-stack planning: stack-state placeholders are a related family but are not needed to satisfy the Step 2 variadic and aggregate-publication acceptance routes.

Step 3 consumers:

- Update `src/backend/prealloc/variadic.hpp` and `src/backend/prealloc/variadic_entry_plans.cpp` to classify variadic helper calls through the call-based contract.
- Update `src/backend/prealloc/call_plans.cpp::call_argument_allows_local_aggregate_address_publication` to reject runtime/intrinsic placeholders through the shared predicate.
- Update `src/backend/prealloc/stack_layout/analysis.cpp::apply_aggregate_address_publication_hints` only if the implementation touches the corresponding publication-hint route; it should use the same predicate as `call_argument_allows_local_aggregate_address_publication` when included.
- Leave `src/backend/prealloc/call_plans.cpp::resolve_direct_callee` behavior-preserving except for any clarifying comment needed to state that placeholder identity is separate from ordinary direct-call identity.

## Suggested Next

Proceed to Step 3 with a behavior-preserving helper API implementation in owned code files: add the call-based runtime placeholder/variadic classifier, route variadic helper classification and aggregate publication through it, and keep direct-call link-name resolution unchanged.

## Watchouts

- Documented compatibility exceptions retained after Step 2: dynamic-stack checks in `dynamic_stack.cpp`, `frame_plan.cpp`, and `regalloc.cpp` may stay string-only for this slice as stack-state runtime compatibility exceptions; `inline_asm` may continue using `CallInst::inline_asm`; AArch64/fabs target intrinsics may continue using `CallInst::intrinsic`.
- The shared predicate must avoid false positives for ordinary user functions whose names happen to begin with `llvm.` when they have a valid `callee_link_name_id`; valid link id means ordinary direct-call identity wins.
- Focused proof expectations for Step 4: include at least one variadic helper classification route, preferably scalar `llvm.va_arg.<type>` plus aggregate `llvm.va_arg.aggregate` or `llvm.va_copy.p0.p0`; include one aggregate-address-publication route showing runtime/intrinsic placeholders reject local aggregate address publication while an ordinary direct call with pointer argument behavior is unchanged.
- If Step 3 finds that `apply_aggregate_address_publication_hints` and `call_argument_allows_local_aggregate_address_publication` diverge, keep both under the same placeholder predicate or report a contract ambiguity instead of inventing a second rule.

## Proof

Targeted source reads confirmed the selected contract fits existing fields in `bir::CallInst`, variadic lowering, variadic helper classification, direct-call resolution, aggregate address publication, and retained stack-state compatibility paths.

Delegated proof:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc && printf 'analysis-only Step 2 proof: no implementation changes under src/backend/bir or src/backend/prealloc\n' > test_after.log
```

Proof log: `test_after.log`.
