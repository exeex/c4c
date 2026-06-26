Status: Active
Source Idea Path: ideas/open/394_rv64_same_module_sret_callee_home_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Narrow Sret Home Publication

# Current Packet

## Just Finished

Steps 3 and 4 added focused coverage and implemented the narrow RV64
same-module callee `sret_param` home-publication route, then corrected the
broader-proof regression and live representative storage mismatch before
acceptance.

Implementation:
- Shared `plan_prepared_formal_publication` semantics were restored: `is_sret`
  formals remain `NoPublication`, so AArch64/x86 do not observe a new global
  formal-publication fact.
- The target-independent stack-layout widening attempt was backed out after it
  moved existing AArch64 frame offsets. The storage-size repair now lives in
  `run_stack_layout()` as an RV64-only stack-object storage contract: pointer
  `sret_param` homes normalize to exact ABI pointer storage (`size=8 align=8`)
  before frame-slot assignment, while AArch64 keeps the shared BIR layout.
- RV64 object admission now fail-closes malformed explicit sret homes and only
  accepts a pointer-typed incoming-register sret formal whose prepared value
  home, frame slot, and `source_kind=sret_param` object agree on a permanent
  pointer-width stack home.
- RV64 formal-entry home emission now stores the ABI-derived incoming sret
  pointer register (`a0` for the supported first sret formal shape) into that
  prepared home before later pointer-value local-memory stores load through it.

Focused coverage:
- `backend_prealloc_formal_publications_test` preserves the shared
  target-neutral contract that varargs and sret formals report
  `NoPublication`.
- `backend_prepare_stack_layout_test` proves the live root cause and the
  larger returned-aggregate metadata guard: pointer-typed `sret_param` BIR
  metadata `size=4 align=4` and `size=16 align=16` both prepare exact 8-byte
  pointer homes on RV64, while AArch64 preserves the shared BIR layout.
- `backend_riscv_object_emission_test` proves the accepted route emits the
  `a0` home store before the later sret pointer load/store consumer, and rejects
  wrong source kind, non-address-exposed/non-permanent object, non-stack home,
  frame-slot mismatch, unsafe 4-byte home, and missing ABI shapes.

Representative evidence:
- Fresh prepared dump:
  `build/agent_state/394_step4_postfix_920908-1.prepared.log`; BIR still shows
  `ptr sret(size=4, align=4) %ret.sret`, but the prepared stack object is now
  `source_kind=sret_param type=ptr size=8 align=8` and preservation/value homes
  use `stack_size=8 stack_align=8`.
- RV64 object representative:
  `build/agent_state/394_step4_postfix_920908-1.case.log` reports
  `[PASS][rv64-gcc-torture-backend-obj]` for
  `tests/c/external/gcc_torture/src/920908-1.c`.

## Suggested Next

Execute Step 5 by rerunning `tests/c/external/gcc_torture/src/920908-1.c`
through the RV64 object representative and classify whether the same-module
sret callee-home segmentation fault is gone or whether a later boundary remains.

## Watchouts

- The repair did not change caller-side `memory_return` address materialization
  or idea 393 aggregate `va_arg` cursor stride.
- Callees with no explicit prepared sret home remain outside this route; the
  fail-closed guard applies when an explicit sret home exists but is malformed.
- The RV64 storage widening is target-specific and runs before frame-slot
  assignment; do not move it back into target-independent stack-object
  collection.
- `prepared_sret_stack_slot_pointer_access` remains the later local-memory
  consumer and does not infer missing entry publication.

## Proof

Delegated proof command run:
`{ cmake --build --preset default --target backend_prealloc_formal_publications_test backend_prepare_stack_layout_test backend_riscv_object_emission_test c4cll; ctest --test-dir build -R 'backend_prealloc_formal_publications|backend_prepare_stack_layout|backend_riscv_object_emission|backend_codegen_route_aarch64_sret_global_scalar_source_publication' --output-on-failure; } > test_after.log 2>&1`

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed
out of 4`, including
`backend_codegen_route_aarch64_sret_global_scalar_source_publication`.
