# Current Packet

Status: Active
Source Idea Path: ideas/open/366_aarch64_loaded_byte_value_reused_as_address.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Address-Carrying Boundary

## Just Finished

Step 1 localized the first bad boundary for idea 366 without code changes.

Semantic/prepared BIR is still correct for the first residual byte load:
`%t6 = bir.load_local ptr %lv.a`, then `%t7 = bir.load_local i8 %t7.addr,
addr %t6`, then `%t8 = bir.sext i8 %t7 to i32`. Prepared addressing records
that as `access block=entry inst_index=9 base=pointer_value result=%t7
pointer=%t6 offset=0 size=1 align=1`, and prepared value locations place
`%t7` at `value_id=7 kind=stack_slot slot_id=29 offset=88` with a
`move_bundle` publishing `from_value_id=7 to_value_id=8` before the consumer
cast.

Generated AArch64 first goes bad immediately after that selected memory load:
`ldr x13, [sp, #16]` loads `%t6`, `ldrb w9, [x13]` computes `%t7`, and
`strb w9, [sp, #88]` publishes `%t7`; the next consumer region emits
`mov x9, x13; ldrb w13, [x9]; sxtb w13, w13` instead of loading `%t7` from
`[sp, #88]` or using an emitted `%t7` scalar register. The same shape repeats
for `%t11`/`%t12` in `for.cond.1`, `%t20`/`%t21` and `%t23`/`%t24` in
`block_1`, and `%t30`/`%t31` in `block_3`.

First bad boundary: AArch64 scalar/control consumer publication after a
prepared pointer-value byte load. The memory-lowering record and printer emit
the load result correctly, but the subsequent scalar consumer falls back to the
load producer's pointer-value address carrier. Likely owning implementation
surface is `src/backend/mir/aarch64/codegen/alu.cpp` scalar/control
publication helpers, especially `append_control_cast_to_register`,
`append_control_value_to_register`, `make_control_publication_operand`, and
`make_unpublished_load_local_source_operand`, together with the emitted-scalar
tracking boundary used after `src/backend/mir/aarch64/codegen/memory.cpp`
`lower_memory_instruction`.

## Suggested Next

Patch the AArch64 scalar/control publication boundary so a consumer of a
prepared load result uses the loaded scalar value home or emitted result
register, and does not rematerialize the load's pointer-value address as the
consumer operand. Start with the Step 2 surface above and keep the fix general
for stack-published scalar load results from pointer-value memory accesses.

## Watchouts

- Do not special-case `00173`, a literal, a stack slot, a register, an emitted
  instruction sequence, or a loop body.
- Do not weaken expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, CTest registration, or external test
  contracts.
- Preserve idea 365 string/global pointer publication and idea 356 dynamic
  pointer-derived byte-load behavior.
- Treat unrelated call lowering, ABI, frontend, runner, timeout, dynamic stack,
  or test-registration evidence as a lifecycle split candidate.
- `make_prepared_scalar_load_source` currently only returns symbol/frame-slot
  scalar sources; pointer-value byte-load consumers need an explicit safe path
  to the load result, not a fallback to the pointer base. Validate nearby
  dynamic `*p`/`*src` byte-load behavior when patching.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j10
--output-on-failure -R '^c_testsuite_aarch64_backend_src_00173_c$' | tee
test_after.log`. Build was up to date; the selected test reproduced the
residual as `[RUNTIME_NONZERO] .../00173.c exit=Segmentation fault`.
Canonical proof log: `test_after.log`.

Backend evidence captured under `/tmp`: `/tmp/00173_dump_bir.txt`,
`/tmp/00173_dump_prepared_bir.txt`, `/tmp/00173_dump_mir.txt`,
`/tmp/00173_trace_mir.txt`, and `/tmp/00173_after_365.s`.
