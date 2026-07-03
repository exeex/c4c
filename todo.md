Status: Active
Source Idea Path: ideas/open/572_rv64_same_module_call_result_lowering.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Review And Close Readiness

# Current Packet

## Just Finished

Step 6 diagnostic close blocker follow-up is complete.

Unsupported ordinary same-module `CallInst` ABI/result shapes now reject with
the call-specific `unsupported_call_abi` diagnostic instead of falling through
to the generic `unsupported_instruction_fragment` diagnostic. The diagnostic
keeps fail-closed behavior and records the function, block, instruction index,
callee, planned argument count, and call result owner context.

Focused backend tests now assert the call-specific diagnostic for unsupported
same-module scalar register results, byval stack-copy arguments, sret memory
returns, scalar stack results, frame-slot value arguments, and frame-slot
address arguments. Non-call producer failures, inline asm, direct extern/probe
call checks, and stack-frame admission failures remain on their existing
diagnostic surfaces.

## Suggested Next

Rerun Step 6 close-readiness for 572 and decide whether to hand the plan to the
plan owner for closure or any close-time regression gate the supervisor wants.

## Watchouts

- This plan remains limited to ordinary same-module RV64 call/result lowering.
- Do not treat `llvm.inline_asm` carriers as ordinary calls.
- Do not implement select, floating-point binary, pointer arithmetic,
  prepared-authority, broad ABI, or runtime-comparison work here.
- Do not add filename-specific matching for `src/20000412-2.c` or
  `src/20000622-1.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Do not use the new `src/20000622-1.c` runtime-abort follow-up as a reason to
  expand 572. The abort is separate from the ordinary same-module call fallback
  repair.
- The diagnostic follow-up did not implement new ABI lowering; it only narrowed
  unsupported ordinary same-module call rejection surfaces.

## Proof

Passed:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Proof log: `test_after.log`.
