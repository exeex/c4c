Status: Active
Source Idea Path: ideas/open/610_rv64_move_bundle_target_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broaden Same-Family Coverage

# Current Packet

## Just Finished

Completed `plan.md` Step 3 diagnostic refresh after the Step 2 direct
register-to-register consumer rule. I re-probed the 106 current
`build/rv64_gcc_c_torture_backend/*/case.log` rows that still named
`unsupported_move_bundle_target_shape` and wrote focused direct object evidence
under `build/agent_state/610_step3_move_bundle_residual_probe/`.

Fresh direct object movement:
- 41 of the 106 stale log-listed rows now compile to RV64 objects, including
  the Step 2 representatives `src/20000314-1.c`, `src/20040309-1.c`,
  `src/pr63641.c`, and the larger mixed evidence case `src/20020206-2.c`.
- 34 rows now fail at downstream non-target-shape owners instead of
  `unsupported_move_bundle_target_shape`: 10 destination fan-in classifier
  rejects, 8 instruction fragments, 3 local-memory accesses, 2 terminator
  fragments, 2 scalar-compare publications, 1 call ABI, 1 inline asm, and 7
  other downstream diagnostics.
- 31 rows still report `unsupported_move_bundle_target_shape`.

The 31 remaining target-shape rows split as follows:
- 13 select-publication rows, not this packet: 7
  `select_publication_rejection_reason=unsupported_source_stack_offset`
  (`src/20000706-1.c`, `src/20000706-2.c`, `src/20000717-5.c`,
  `src/20071213-1.c`, `src/20120427-1.c`, `src/20120427-2.c`,
  `src/991216-1.c`), 4
  `select_publication_rejection_reason=intent_status_unsupported_source_home`
  (`src/pr45034.c`, `src/pr53160.c`, `src/pr58726.c`, `src/pr59221.c`), and 2
  `select_publication_rejection_reason=unsupported_source_immediate_i32_range`
  (`src/pr29695-1.c`, `src/pr29695-2.c`).
- 12 before-instruction stack-destination multi-source rows with
  `authority=none`, `move_count=3`, register-to-stack plus stack-to-stack
  moves, and the same destination value; these belong with destination fan-in
  or before-instruction generic stack destination policy, not Step 3 direct
  out-of-SSA materialization.
- 2 before-return ABI rows (`src/20001130-2.c`, `src/20080719-1.c`), outside
  this plan's Step 3 route.
- 2 before-instruction generic stack-destination rows (`src/920411-1.c`,
  `src/990829-1.c`), outside the out-of-SSA parallel-copy same-family route.
- 1 evidence-gap row (`src/pr47337.c`) whose diagnostic still lacks the
  published coordinate and move facts needed for safe ownership.
- 1 same-family `out_of_ssa_parallel_copy` mixed multi-move row,
  `src/pr71631.c`, with `move_count=10`, complete published prepared homes for
  register-to-register, register-to-stack, and rematerializable-immediate to
  register moves. It is the only remaining non-select out-of-SSA target-shape
  row, but it also includes repeated stack destinations, so the next code
  packet must keep destination fan-in fail-closed.

## Suggested Next

Continue with `plan.md` Step 3 by implementing a narrow same-family
multi-move consumer for `out_of_ssa_parallel_copy` bundles only when all
scheduled moves have complete prepared homes and the bundle has no repeated
destination/fan-in. The exact rule should broaden the existing direct
register-to-register consumer to acyclic multi-move bundles containing only
direct `register -> register`, direct `register -> stack_slot`, and
`rematerializable_immediate -> register` scalar moves, preserving fail-closed
diagnostics for repeated stack destinations.

Use `src/pr71631.c` only as audit evidence for the mixed multi-move shape; do
not force it to pass if its repeated stack destinations remain unresolved.
Exact proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Watchouts

- Do not infer missing prepared authority from an encodable RV64 move; continue
  consuming only prepared move-bundle facts.
- Keep destination fan-in policy out of this plan. `src/960209-1.c` and the 10
  fresh `unsupported_prepared_move_bundle_classification` reroutes are
  explicitly in the non-parallel multi-source stack-destination family, and
  the 12 remaining before-instruction target-shape rows have the same
  stack-destination/fan-in character.
- Do not touch expectations, unsupported markers, allowlists, timeout files,
  runtime accounting, or unrelated implementation surfaces.
- Keep select-publication immediate materialization, before-return ABI moves,
  before-instruction generic moves, unresolved stack destinations, and
  evidence-gap rows out of this direct register-to-register rule.
- The current fresh evidence has no clean remaining single-move
  `phi_join_register_to_register` row. If the next implementation cannot
  distinguish safe acyclic multi-move bundles from repeated-destination
  bundles without guessing, stop and report that missing authority instead of
  adding testcase-shaped handling.
- The CTest backend subset is the canonical proof log, but focused direct
  `c4cll --codegen obj --target riscv64-linux-gnu` probes were needed to
  observe row movement because the existing per-case logs were not the
  acceptance artifact for this packet.

## Proof

Proof command run for this diagnostic packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed. Backend subset used: `^backend_`. Log path:
`test_after.log`. The proof is sufficient for this Step 3 diagnostic slice.
Focused direct object probe evidence lives under
`build/agent_state/610_step3_move_bundle_residual_probe/`.
