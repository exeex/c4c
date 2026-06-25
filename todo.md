Status: Active
Source Idea Path: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Review and Handoff Back to 356

# Current Packet

## Just Finished

Step 5 is complete for the idea-359 review/handoff back to idea 356.

The shared prepared object consumer contract is now independently useful and
covered by the focused proof set. It gives target consumers a common traversal
schedule, prepared move-bundle query, select-carrier classifier, value-home
query, frame-slot query, and diagnostic helper. AArch64 traversal and RV64
prepared object emission both consume the shared critical-edge parallel-copy
obligation diagnostic instead of rediscovering skipped obligations locally.

Idea 359 unblocks these idea 356 blockers:

- RV64 object-route work no longer needs to infer block-entry versus
  predecessor-terminator copy placement from target-local phase names.
- RV64 can reject unsupported critical-edge parallel-copy placement through the
  shared `UnsupportedParallelCopyExecutionSite` diagnostic path while later
  multi-block work adds real placement support.
- Target consumers have shared fail-closed helpers for select/join-transfer
  carriers, value homes, move bundles, frame slots, and prepared-consumer
  diagnostics, so 356 can focus on target-block emission, labels, fixups,
  relocations, and instruction coverage.
- The accepted AArch64 and RV64 hook slices prove the intended backend shape:
  target code provides concrete emission/diagnostic hooks while BIR/prepared
  remains the authority for traversal and data-flow placement.

Idea 359 leaves these idea 356 blockers explicitly out of scope:

- Globals, strings, and data-section object emission.
- Vararg and FPR ABI coverage.
- Broad RV64 scalar instruction coverage and testcase-by-testcase opcode
  expansion.
- Removing the remaining RV64 object-route architecture blockers around
  multi-block target streams, labels, branch/call fixups, relocations, and
  executable gcc-torture/qemu progress.
- The retracted `20000113-1.c` slice remains unaccepted as a target-local fix.

## Suggested Next

Suggested next for the supervisor/plan-owner: treat idea 359 as closure-ready
after reviewing this handoff against the source acceptance criteria, then
reactivate idea 356 rather than create a separate follow-up idea. The first 356
packet should be narrow: re-audit the representative RV64 multi-block object
route cases against the now-shared prepared consumer contract and identify the
next target-owned blocker in target-block preservation, MIR pseudo lowering,
assembler-backed encoding, labels/fixups/relocations, or missing RV64
instruction/ABI support.

## Watchouts

- Closing idea 359 should not be treated as completing idea 356; it only clears
  the shared traversal/query/diagnostic precondition that had blocked clean
  RV64 object-route work.
- The next 356 packet should still reject testcase-name shortcuts,
  expectation weakening, target-local CFG reconstruction, and accepting
  `20000113-1.c` without proving the architecture through the representative
  route.
- RV64 prepared object emission still has a diagnostic-capable builder beside
  the legacy optional-return builder; 356 may need to decide when top-level
  object-route diagnostics should stop collapsing optional failure into a
  coarse unsupported-shape message.
- AArch64 traversal remains a useful cross-target guard for the shared
  contract, but 356 acceptance must be RV64 object-route executable progress,
  not just prepared-consumer helper coverage.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(riscv_object_emission|prepared_object_consumer_contract|aarch64_function_traversal|prepare_phi_materialize|prepared_lookup_helper|prepared_printer)$' > test_after.log 2>&1`

Proof log: `test_after.log`.
