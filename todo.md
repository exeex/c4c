Status: Active
Source Idea Path: ideas/open/382_aarch64_dynamic_stack_vla_fixed_slot_addressing.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Fixed-Slot Addressing

# Current Packet

## Just Finished

Step 3 completed the AArch64 dynamic-stack fixed-slot repair. Fixed stack homes
now consistently route through the prepared stable frame anchor in the memory,
address materialization, local-address, fixed-formal, select/compare temporary
publication, and scalar ALU stack-publication paths.

The missing continuation from supervisor review was the scalar ALU stack
publication path: `00207.c.s` had `str w9, [sp, #24]` followed by
`ldr w9, [x29, #24]`. That store now emits `str w9, [x29, #24]`.

Dynamic-stack functions that require fixed slots to use `x29` now also get a
printable frame boundary: the prologue allocates the fixed frame, saves/restores
`x29` and `x30` when needed, anchors `x29` before dynamic `sp` movement, and the
epilogue restores from `x29` before returning.

Follow-up regression repair: restored the existing non-leaf LR frame reservation
contract while keeping the dynamic-stack `x29` anchor reservation separate.
`backend_aarch64_return_lowering` still sees the saved-register non-leaf frame
preserve LR at offset 8 with the established 32-byte frame.

## Suggested Next

Supervisor should review the Step 3 slice for acceptance and choose whether to
commit this code plus `todo.md`, or run broader AArch64 backend validation first.

## Watchouts

- The repair is policy-driven from the prepared frame plan; no filename,
  offset, or instruction-neighborhood shortcut was added.
- Dynamic-stack fixed-slot frame nodes now intentionally tolerate prepared
  callee-saved entries that do not yet have printable slot placements; this
  preserves the existing register-preservation route while adding the required
  `x29`/`x30` frame boundary for functions like `00207`'s `f1`.
- Broader validation may still be useful because this touched shared AArch64
  scalar publication and frame printing paths.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_return_lowering|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00207_c)$' > test_after.log 2>&1`

Result: build succeeded; all three selected CTests passed. `test_after.log` is
the canonical proof log. Fresh `build/c_testsuite_aarch64_backend/src/00207.c.s`
uses `str w9, [x29, #24]` with `ldr w9, [x29, #24]`, includes the dynamic-stack
frame anchor prologue/epilogue, and no longer times out with repeated `boom!`.

Supervisor backend regression guard also passed with matching commands:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_before.log 2>&1`

then, after restoring this slice:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Guard result: before 148 passed / 0 failed / 148 total; after 149 passed /
0 failed / 149 total; no new failures.
