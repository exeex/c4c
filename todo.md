Status: Active
Source Idea Path: ideas/open/348_vrm_regalloc_mir_and_rv64_assembler_bridge.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: RV64 Assembler/Object Bridge And Milestone Proof

# Current Packet

## Just Finished

Completed Step 5: RV64 Assembler/Object Bridge And Milestone Proof. A
source-level `__c4c_builtin_vrm2` positional `.insn.d` inline-asm fixture now
compiles through `--codegen obj --target riscv64-linux-gnu` into c4c-owned RV64
ELF object emission. The object-route test checks the deterministic native text
bytes for the EV `.insn.d` instruction plus the source `return 0` tail, while
the existing prepared `.insn.d`, scalar `.insn r`, VRM allocation, and
scalar-does-not-satisfy-VRM proofs remain green.

## Suggested Next

Ask the supervisor to run the broader milestone regression guard or route the
plan owner for closure review, since the source-level VRM `.insn.d` object-byte
proof is now present.

## Watchouts

- No VRM function-call ABI behavior was added.
- Source inline asm reaches object emission through frontend canonical `${N}`
  placeholders, semantic BIR, prealloc homes, and the native RV64 object route;
  the proof does not use an external assembler.
- VRM local loads/stores remain zero-size register-carrier bookkeeping in the
  object walker once prepared homes have been assigned.
- Scalar `.insn r` object encoding remains GPR-only, and scalar integer values
  still fail vector-register carrier compatibility through the existing
  `backend_prepare_liveness` negative fixture.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_cli_riscv64_vrm_insn_d_source_obj|backend_prepare_liveness|backend_lir_to_bir_notes)$'`

Proof log: `test_after.log`.
