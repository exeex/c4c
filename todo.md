Status: Active
Source Idea Path: ideas/open/377_aarch64_external_libc_call_result_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Scalar External-Call Result Publication

# Current Packet

## Just Finished

Step 2: Repair Scalar External-Call Result Publication now has focused backend
coverage for scalar GPR call results whose prepared destination home is a stack
slot.

`backend_aarch64_call_boundary_owner` now includes
`scalar_call_result_publishes_gpr_to_prepared_stack_home`, a generic
`produce_count` fixture that drives `lower_after_call_moves` directly and
requires a structured `MemoryInstructionRecord` store from ABI `x0` into a
prepared frame-slot home at offset 40. This covers the pre-repair failure shape
where the stack-slot call-result branch returned no publication instruction,
without encoding `00187`, `fread`, or offset 96.

For `00187`, generated `00187.c.s` now publishes the `fread` return count with
`str x0, [sp, #96]` immediately after `bl fread`, before the existing
`ldr x9, [sp, #96]` / `cmp x9, #6` consumer sequence. The delegated `00187`
CTest now passes, so this packet did not advance to a new first bad fact.

## Suggested Next

Supervisor should decide whether Step 2 is sufficient to accept or whether a
review/broader AArch64 call-boundary subset is needed before selecting the next
packet.

## Watchouts

The focused coverage asserts the selected machine record shape rather than
printed assembly, so it guards the call-boundary lowering contract before the
printer route. Prepared stack-slot call-result plans record
`destination_register_bank=none` even when the storage value is GPR-backed, so
the lowering key remains the ABI source bank plus destination storage/home
shape, not destination register-bank metadata.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00187_c)$'; } > test_after.log 2>&1`

Result: passed after adding the focused call-boundary coverage. Build succeeded;
`backend_aarch64_call_boundary_owner`, `backend_aarch64_instruction_dispatch`,
and `c_testsuite_aarch64_backend_src_00187_c` all passed. Proof log:
`test_after.log`.
