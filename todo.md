Status: Active
Source Idea Path: ideas/open/293_backend_remaining_aarch64_load_local_memory_failures.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Add focused backend proof for the admitted shape

# Current Packet

## Just Finished

Step 4: added a focused shape-level proof in `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` for the newly admitted opaque runtime pointer-value local-memory access path.

The new `runtime_pointer_value_opaque_i32_access` fixture uses a synthetic runtime `char*` formal parameter and proves both an `i32` load and store lower as `LoadLocalInst`/`StoreLocalInst` with `MemoryAddress::BaseKind::PointerValue`, formal-parameter provenance, and `MemoryLayoutAuthorityKind::OpaqueCompatibility`. The existing casted byte-pointer `i32` fail-closed note guards remain in the same `expect_indirect_local_memory_lvalue_contracts` coverage path.

## Suggested Next

Supervisor should decide whether this completed proof slice is enough to accept Step 4 or whether to run broader route validation/reviewer scrutiny before choosing the next lifecycle step.

## Watchouts

- The focused proof is independent of the three named AArch64 route fixtures and observes the BIR shape directly.
- The test intentionally keeps the broad casted byte-pointer `i32` local update/store cases fail-closed through the existing note checks.
- No implementation files, route fixture expectations, labels, `plan.md`, or source idea files were changed in this packet.

## Proof

Command:

```bash
cmake --build --preset default; ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$' > test_after.log 2>&1
```

Outcome: build completed and the delegated `backend_lir_to_bir_notes` CTest passed. `test_after.log` is the proof log.
