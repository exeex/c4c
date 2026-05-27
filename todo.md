# Current Packet

Status: Active
Source Idea Path: ideas/open/57_aarch64_variadic_va_arg_register_save_progression_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair register-save source address ordering

## Just Finished

Completed Step 2 implementation for aggregate `va_arg` register-save source
address ordering in `src/backend/mir/aarch64/codegen/variadic.cpp`.

The shared aggregate register-save copy helper now accepts an already-loaded
pre-increment progression offset register. The `RegisterSaveArea` branch loads
the current `gp_offset`/`fp_offset` with `ldrsw`, computes the advanced value in
the other scratch register for the overflow decision, uses the sign-extended
old offset for the current aggregate source address, and stores the advanced
offset only after the register-save copy. This covers both
`GpRegisterSaveArea` and `FpRegisterSaveArea` aggregate paths through the shared
helper. The overflow-area fallback still passes no preloaded offset and keeps
its pointer-field copy/update behavior.

## Suggested Next

Classify the remaining `00204` stdarg string corruption after the aggregate
register-save ordering fix. The generated aggregate HFA register-save paths now
use the old sign-extended offset before storing the advanced offset, while the
focused proof still fails in the string-pointer stdarg output, so the next
packet should inspect the non-aggregate variadic string/pointer route rather
than reworking aggregate HFA copy ordering.

## Watchouts

The register-save offset must remain sign-extended in the X register; preserving
only the W register would turn negative AAPCS64 offsets into zero-extended
addresses. The helper still shares `address_scratch` for source-address
formation and destination-address fallback materialization, so broad acceptance
should keep an eye on unusually large destination offsets, even though the
current aggregate register-save lane offsets are small.

Treat existing dirty `memory.cpp`, `dispatch_edge_copies.cpp`, and transient
`review/*` files as external context.

## Proof

Ran the delegated proof:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build passed, focused subset `7/8` passed, and only
`c_testsuite_aarch64_backend_src_00204_c` failed with `[RUNTIME_MISMATCH]`.
Generated `00204.c.s` confirms the aggregate register-save path now emits
`ldrsw x10, [x21, #28]`, uses `x10` in the source address before the copy, and
stores the advanced offset after the copy.
Proof log path: `test_after.log`.
