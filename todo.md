Status: Active
Source Idea Path: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate The RV64 Call-Argument Lowering Boundary

# Current Packet

## Just Finished

Step 2 boundary inspection completed for prepared
`arg.source_selection=local_frame_address_materialization` using the Step 1
evidence summary at
`build/agent_state/648_step1_call_arg_frame_slot_evidence/summary.md` and
AST-backed lookup over the RV64 call emission files.

The owned implementation surface is the RV64 prepared call-argument consumer:

- text asm path:
  `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`,
  `emit_riscv_simple_call` and the helper
  `emit_riscv_frame_slot_address_argument`
- object path:
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `fragment_for_prepared_call`,
  `append_rv64_prepared_local_frame_address_call_argument_source`, and
  `prepared_frame_slot_address_call_argument_offset`

The stale register-copy path is the final ordinary GPR register-source fallback
in both consumers. In the text path, `emit_riscv_simple_call` first probes
`as_local_frame_address_materialization_route` and emits `addi <arg>, sp, off`;
if the source selection is malformed but still
`LocalFrameAddressMaterialization`, it fails closed before the ordinary
register copy. In the object path, `fragment_for_prepared_call` has a matching
`LocalFrameAddressMaterialization` branch before the ordinary
`append_rv64_move` register-source fallback.

The narrow implementation surface for Step 4 should stay inside those branches
and their shared contract/offset helper logic. Do not repair this by changing
generic move-bundle production, stack layout, ABI register assignment, source
syntax handling, string-label pointer admission, or testcase expectations.

The fresh `unsupported_local_memory_access` object-route blocker for
`tests/c/external/gcc_torture/src/20000722-1.c` is not the same
call-argument consumption boundary. It is produced by
`diagnose_unsupported_prepared_instruction_fragment` while rejecting a local
load/store instruction before object emission reaches a fresh disassembly
proof for the call setup. Treat it as a representative-row integration blocker
that must be split or handled before Step 5 object/disassembly proof; it should
not block focused Step 3 coverage of the call-argument consumer.

## Suggested Next

Execute Step 3 by adding focused backend coverage for the prepared
call-argument consumer, not the GCC torture row. Recommended narrow target:
`tests/backend/case/riscv64_call_arg_local_frame_address_materialization.c`,
wired to a CTest that observes prepared/RV64 lowering for a call argument with
`arg.source_selection=local_frame_address_materialization`.

The positive assertion should fail on the old stale register-home-copy shape:
the selected frame-slot address must be materialized into the ABI GPR argument
register with an `addi <dest>, sp, <selected offset>`-style setup, and the
focused observable must not accept a `call_arg_register_to_register` or plain
source-register `mv` as satisfying that selected-source contract.

The negative assertion should preserve fail-closed behavior for malformed,
missing, ambiguous, or non-frame-slot source selections: they must not fall
through to an inferred stack offset or source-register copy merely because the
value happens to have a register home or stack-looking metadata.

## Watchouts

- Do not reopen string-constant local-memory admission.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `s2`, or `a0`.
- Do not infer frame-slot address materialization from source spelling, stack
  offsets, final assembly, testcase identity, or diagnostics.
- Treat `reason=call_arg_register_to_register` as the stale prepared
  move-bundle path, but do not claim a fresh emitted `mv a0,s2` unless the
  object route reaches disassembly again.
- The fresh representative object route currently stops at
  `unsupported_local_memory_access` outside the call-argument consumer. Do not
  broaden Step 3/4 into local-memory admission just to get
  `src/20000722-1.c` to object disassembly.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.
- Preserve ordinary register-to-register GPR call-argument lowering when there
  is no explicit `LocalFrameAddressMaterialization` source selection.

## Proof

No build/ctest proof was required for this read-only boundary-location packet.
Read `build/agent_state/648_step1_call_arg_frame_slot_evidence/summary.md` and
inspected `prepared_call_emit.cpp`, `object_emission.cpp`, and directly related
helpers using `c4c-clang-tool-ccdb` plus focused source reads. Did not create
or overwrite `test_after.log`.
