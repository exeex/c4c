Status: Active
Source Idea Path: ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Run Broader Validation And Close Readiness

# Current Packet

## Just Finished

Completed Step 5 by running the supervisor-delegated broader validation subset and recording close-readiness for the runtime placeholder identity contract.

Final contract:

- Runtime/intrinsic placeholder calls are identified through shared call-based helpers over `bir::CallInst`: a placeholder must be a non-indirect call with invalid `callee_link_name_id` and no `callee_value`; structured `intrinsic`/`inline_asm` metadata is authoritative, while invalid-link `llvm.*` display names remain a compatibility placeholder path.
- Ordinary direct calls keep valid `callee_link_name_id` as their authority and do not fall back to raw callee text.
- `prepared_variadic_entry_helper_kind_for_call()` is the variadic helper query surface for placeholder calls.
- `call_is_runtime_intrinsic_placeholder()` is the aggregate-address-publication exclusion used by prepared call plans and stack-layout publication hints.

Touched consumers:

- `src/backend/prealloc/variadic_entry_plans.cpp` now classifies variadic runtime helper entry plans through the shared call-based helper.
- `src/backend/prealloc/call_plans.cpp` now blocks local aggregate address publication through the shared placeholder predicate instead of an undocumented raw `llvm.` prefix check.
- `src/backend/prealloc/stack_layout/analysis.cpp` now skips aggregate publication hints for the same placeholder predicate.

Focused Step 4 proofs:

- `backend_prealloc_call_boundary_classification` covers placeholder helper classification and rejects valid linked direct calls plus indirect display-name calls.
- `backend_prepare_liveness` covers prepared call-plan aggregate publication, keeping valid linked `llvm.user_sink` calls eligible while rejecting raw `llvm.va_start.p0` compatibility placeholders and structured intrinsic placeholders.

## Suggested Next

Ask plan-owner to evaluate close for `ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md`; the executor-visible runbook steps are complete.

## Watchouts

- Retained raw `llvm.*` matching is documented compatibility identity for invalid-link runtime placeholders only; it is not ordinary direct-call semantic authority.
- Dynamic-stack raw string checks remain compatibility exceptions outside this contract's changed consumers.
- No separate adjacent initiative was discovered during broader validation.
- No implementation or test files were edited in this packet.

## Proof

Delegated proof:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R 'backend_(prepare_liveness|prepare_stack_layout|prealloc_call_boundary_classification|prepare_frame_stack_call_contract|aarch64_call_boundary_owner|codegen_route_aarch64_variadic_aggregate_overflow_byte_copy)' >> test_after.log 2>&1
```

Result: passed. Build reported no work to do, then CTest passed 6/6 tests:

- `backend_prealloc_call_boundary_classification`
- `backend_prepare_liveness`
- `backend_prepare_stack_layout`
- `backend_prepare_frame_stack_call_contract`
- `backend_aarch64_call_boundary_owner`
- `backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy`

Proof log: `test_after.log`.
