Status: Active
Source Idea Path: ideas/open/293_backend_remaining_aarch64_load_local_memory_failures.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Locate the semantic BIR admission boundary

# Current Packet

## Just Finished

Step 2: located the semantic BIR admission boundary for the classified opaque runtime pointer local-memory loads without implementation edits.

The rejecting rule is in `src/backend/bir/lir_to_bir/memory/provenance.cpp`:

- `try_lower_addressed_pointer_load` calls `classify_scalar_subobject_addressability(..., allow_opaque_ptr_base=true)`.
- `classify_scalar_subobject_addressability` returns `ScalarSubobjectAddressability::OpaqueCompatibility` for a non-negative offset, non-zero access size, `stored_type == Void`, `byte_offset == 0`, and empty/`ptr`/`i8` `type_text`.
- `try_lower_addressed_pointer_load` then immediately rejects that verdict with `if (addressability == ScalarSubobjectAddressability::OpaqueCompatibility) return false;`.
- `try_lower_addressed_pointer_store` has the same explicit `OpaqueCompatibility` rejection before emitting `StoreLocalInst`.

Nearby accepted local-memory load/store shapes already include exact typed scalar access (`stored_type == access_type`), byte-wise `i8` inspection/update of a known scalar object representation, scalar access covered by byte-storage aggregate layout, scalar-subobject access resolved through layout facts, pointer-value loads from `local_slot_pointer_values` when the pointer slot itself carries `Ptr` and the access is non-pointer, dynamic pointer-value array loads when the dynamic element type matches the requested value type, and the special AArch64 variadic FP lane load path.

The smallest intended generalization is not to make all `OpaqueCompatibility` addressable. It should admit only runtime pointer-value local-memory access where the pointer-value side table proves an addressed runtime base and the access is represented as a BIR `LoadLocalInst`/`StoreLocalInst` with `MemoryAddress::BaseKind::PointerValue`, preserving provenance via `pointer_value_memory_provenance_with_layout_authority` and marking layout authority/range verdict as opaque compatibility. The byval subcase needs `load i8` through a dynamic `i8*` byte pointer. The writeback subcase needs `load i32` and later `store i32` through a reloaded `char *` pointer value. Both should remain shape/provenance rules, not fixture-name rules.

## Suggested Next

Proceed to Step 3 with a narrow implementation packet in `src/backend/bir/lir_to_bir/memory/provenance.cpp`: add a helper around the addressed-pointer load/store `OpaqueCompatibility` gate that admits only the intended runtime pointer-value local-memory shapes, then run the delegated three-test AArch64 subset.

## Watchouts

- Do not weaken expectations, labels, snippets, or semantic BIR fail-closed behavior to make the named failures pass.
- Do not add named-case shortcuts for `same_bytes`, `main`, `byval_helper_payload_*`, or `aarch64_pointer_value_named_scalar_writeback`.
- Preserve the 286/288/291/292 prepared/interface cleanup behavior called out in the source idea.
- Existing note tests `expect_casted_byte_pointer_i32_update_fails_closed` and `expect_casted_byte_pointer_i32_store_fails_closed` currently assert that casted byte-pointer `i32` opaque compatibility load/store fail closed before BIR publication. If the Step 3 repair intentionally admits the writeback `i32` load/store shape, update or split that proof deliberately; do not accidentally erase this guardrail.
- If byval `i8` dynamic byte loads and writeback `i32` char-derived loads/stores cannot share one precise provenance predicate, split the writeback family into a follow-up rather than broadening the opaque compatibility gate.

## Proof

Command:

```bash
cmake --build --preset default; ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_aarch64_(byval_payload_(8_to_13|9_to_14)_stack_overflow|pointer_value_named_scalar_writeback_uses_computed_store_value)' > test_after.log 2>&1
```

Outcome: build was up to date; CTest failed 0/3 as expected for this no-implementation boundary-location packet. The two byval tests still fail in `same_bytes` with `load local-memory semantic family`, and the writeback test still fails in `main` with `load local-memory semantic family`. `test_after.log` is the proof log.
