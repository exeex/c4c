Status: Active
Source Idea Path: ideas/open/293_backend_remaining_aarch64_load_local_memory_failures.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce and classify the three failures

# Current Packet

## Just Finished

Step 1: reproduced the delegated three-test AArch64 backend subset and classified the first rejected local-memory load shapes without implementation edits.

The two byval stack-overflow tests fail in the same `same_bytes` helper shape. LLVM/LIR reaches:

```llvm
%t8 = getelementptr i8, ptr %p.lhs, i64 %t7
%t9 = load i8, ptr %t8
...
%t12 = getelementptr i8, ptr %p.rhs, i64 %t11
%t13 = load i8, ptr %t12
```

The semantic GEP path can publish `%t8`/`%t12` as runtime pointer values, but the following `load i8` is rejected because the addressed pointer has `stored_type=Void` and `type_text=i8`, which classifies as opaque compatibility at the addressed-pointer load boundary.

The pointer-value writeback test fails in `main` on:

```llvm
%t8 = load ptr, ptr %lv.data
%t9 = load i32, ptr %t8
...
store i32 %t12, ptr %t8
```

Here `%lv.data` stores `writeback_data + 4`; the pointer-slot reload preserves a runtime pointer value with `type_text=i8`, then the `load i32` through that opaque byte pointer is rejected at the same addressed-pointer load boundary. The three failures therefore share one semantic local-memory admission boundary around loads through opaque runtime pointer values, with two access-width subcases: exact byte inspection for `same_bytes`, and typed wider read/write through a char-derived pointer for writeback.

## Suggested Next

Proceed to Step 2: locate the smallest semantic BIR admission rule in the addressed-pointer load/store provenance path that can admit opaque runtime pointer-value local-memory access without weakening existing fail-closed compatibility gates.

## Watchouts

- Do not weaken expectations, labels, snippets, or semantic BIR fail-closed behavior to make the named failures pass.
- Do not add named-case shortcuts for `same_bytes`, `main`, `byval_helper_payload_*`, or `aarch64_pointer_value_named_scalar_writeback`.
- Preserve the 286/288/291/292 prepared/interface cleanup behavior called out in the source idea.
- The byval family needs `load i8` through a dynamic `i8*` parameter/index pointer; the writeback family needs `load i32` and later `store i32` through a reloaded `char *` pointer value. Treat this as a shape/provenance rule, not as approval to admit arbitrary opaque pointer loads.

## Proof

Command:

```bash
cmake --build --preset default; ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_aarch64_(byval_payload_(8_to_13|9_to_14)_stack_overflow|pointer_value_named_scalar_writeback_uses_computed_store_value)' > test_after.log 2>&1
```

Outcome: build was up to date; CTest failed 0/3 as expected for classification. `test_after.log` is the proof log. The supervisor-selected proof was sufficient for reproduction/classification but is intentionally still red because this packet made no implementation edits.
