# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.2.2
Current Step Title: Migrate Canonical Call Issuance, Cleanup, And Result Publication
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Completed step 2.2.2 for the remaining canonical call cleanup and
result-publication helpers still owned by mixed lowering by moving
`emit_call_cleanup_impl`, `emit_call_store_result_impl`, and
`emit_call_store_i128_result_impl` out of
`lowering/call_lowering.cpp` and into reviewed compiled owner
`core/x86_codegen_output.cpp`. `call_lowering.cpp` now keeps call issuance and
classification flow while the pure post-call stack cleanup and result
publication asm helpers live behind the compiled output seam.

## Suggested Next

Stay on step 2.2.2 only if there is still a remaining pure call/return asm
helper family outside the compiled owner, likely in a separate
`return_lowering.cpp` packet such as `emit_return_i128_to_regs_impl`; otherwise
have the supervisor assess whether the mixed call-lowering ownership targeted
by this step is now exhausted.

## Watchouts

- The backend target still does not compile `shared_call_support.cpp` or
  `mod.cpp`; seam moves have to remain self-contained in compiled reviewed
  owners rather than re-linking dormant utilities.
- `call_lowering.cpp` still owns the actual call issuance, ABI classification,
  and prepared-route policy flow; this packet moved only pure cleanup/result
  publication asm helpers into `core/x86_codegen_output.cpp`.
- `shared_call_support.cpp` still carries dormant support definitions; do not
  treat this packet as permission to revive that file as an owner.
- Keep `call_lowering.cpp`, `memory.cpp`, and `mod.cpp` non-owning for this
  route; do not widen into ABI policy, prepared-route admission logic, or
  dormant utility reattachment.

## Proof

Step 2.2.2 canonical call cleanup/result-publication helper migration on
2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
