# Current Packet

Status: Active
Source Idea Path: ideas/open/366_aarch64_loaded_byte_value_reused_as_address.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Scalar Value Versus Address Carrier Handling

## Just Finished

Step 2 repaired the scalar value versus address carrier boundary in
`src/backend/mir/aarch64/codegen/dispatch.cpp`. `emit_value_publication_to_register`
now treats same-block `LoadLocalInst` producers like scalar values first when a
fresh memory reload was not requested: if the load result has a prepared scalar
home, publication reloads that result home instead of rematerializing the
load's pointer-value address.

The focused dispatch test now asserts the repaired shape for a pointer-derived
byte load consumed as a call argument: the consumer reloads the scalar byte from
its prepared stack home, moves the widened value to `w0`, calls `sink_i32`, and
does not emit the stale `ldrb w13, [x9]` loaded-byte-as-address form. The
delegated C testsuite reproducer `c_testsuite_aarch64_backend_src_00173_c`
passes with the same repair.

## Suggested Next

Supervisor should review and commit the completed Step 2 slice, then decide
whether the active idea needs a broader regression guard or lifecycle closure.

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
- The repair intentionally keeps `reload_current_memory_loads` authoritative;
  callers that need a fresh current memory load can still bypass the scalar
  result home path.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j10
--output-on-failure -R
'^(backend_lir_to_bir_notes|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00173_c)$'
| tee test_after.log`. Build succeeded and all 4 selected tests passed.
Canonical proof log: `test_after.log`.
