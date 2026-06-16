Status: Active
Source Idea Path: ideas/open/293_backend_remaining_aarch64_load_local_memory_failures.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Implement one coherent semantic admission repair

# Current Packet

## Just Finished

Step 3: implemented the semantic admission repair in `src/backend/bir/lir_to_bir/memory/provenance.cpp`.

The new rule keeps `OpaqueCompatibility` rejected by default, but admits addressed pointer-value `LoadLocalInst`/`StoreLocalInst` shapes when the side-table `PointerAddress` proves a narrow runtime-compatible local-memory access: base is a named pointer value, pointer offset is zero, the pointed shape is opaque/byte-like (`Void` or `I8` with empty/`ptr`/`i8` type text), and the access is either an `i8` byte load/store, a wider access over runtime-capable provenance (`UnknownRuntimeBase`, global/formal/byval/sret), or an unknown-provenance byte-storage aggregate that covers the scalar access. Emitted memory operations still use `MemoryAddress::BaseKind::PointerValue` and `pointer_value_memory_provenance_with_layout_authority`, so opaque-compatible admissions carry `OpaqueCompatibility` layout authority and `UnknownCompatible` range verdict.

## Suggested Next

Proceed to Step 4 with supervisor-selected broader validation or review for the now-green three-test family. Include the existing note guard if the supervisor wants explicit proof that casted byte-pointer `i32` local cases still fail closed.

## Watchouts

- The repair intentionally admits unknown-provenance wider accesses only through byte-storage aggregate coverage, not through a blanket `OpaqueCompatibility` gate.
- The focused `backend_lir_to_bir_notes_test` binary passed after the change, preserving `expect_casted_byte_pointer_i32_update_fails_closed` and `expect_casted_byte_pointer_i32_store_fails_closed`.
- No tests, expected snippets, labels, `plan.md`, or source idea files were changed.

## Proof

Command:

```bash
cmake --build --preset default; ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_aarch64_(byval_payload_(8_to_13|9_to_14)_stack_overflow|pointer_value_named_scalar_writeback_uses_computed_store_value)' > test_after.log 2>&1
```

Outcome: build completed and the delegated subset passed 3/3. `test_after.log` is the proof log.

Additional focused guard:

```bash
./build/tests/backend/bir/backend_lir_to_bir_notes_test
```

Outcome: passed.
