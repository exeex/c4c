Status: Active
Source Idea Path: ideas/open/610_rv64_move_bundle_target_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The First Supported RV64 Materialization Rule

# Current Packet

## Just Finished

Completed `plan.md` Step 2: added the first supported RV64 materialization rule
for authorized single-move
`pre_terminator_copies/block_entry/out_of_ssa_parallel_copy`
`phi_join_register_to_register` rows with explicit prepared register source
and destination homes. The rule emits the direct GPR move before the
select-edge producer rematerialization helper can reject a row whose ordinary
prepared source home is already a register.

Focused direct object probes after the rule:
- `src/20000314-1.c`, `src/20040309-1.c`, and `src/pr63641.c` now compile to
  RV64 objects.
- `src/960209-1.c` advances past the prior
  `unsupported_move_bundle_target_shape` phi-join stop and now fails later at
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  for an unrelated before-instruction stack fan-in bundle.
- `src/20020206-2.c` also compiles to an RV64 object after the direct phi-join
  consumer rule plus existing edge-preservation handling, but it was not used
  as the first proof target because it is a larger mixed bundle.

## Suggested Next

Continue with `plan.md` Step 3 by broadening only within the same prepared
out-of-SSA move-bundle authority contract. The next coherent packet is to
diagnose the remaining `unsupported_move_bundle_target_shape` rows after the
direct phi-join rule, separating same-family multi-move or stack-destination
consumer gaps from destination fan-in, select immediate publication,
before-return ABI, before-instruction generic moves, and evidence-gap rows.
Exact proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Watchouts

- Do not infer missing prepared authority from an encodable RV64 move; continue
  consuming only prepared move-bundle facts.
- Keep destination fan-in policy out of this plan. `src/960209-1.c` now exposes
  a separate before-instruction non-parallel multi-source stack-destination
  blocker after the Step 2 movement.
- Do not touch expectations, unsupported markers, allowlists, timeout files,
  runtime accounting, or unrelated implementation surfaces.
- Keep select-publication immediate materialization, before-return ABI moves,
  before-instruction generic moves, unresolved stack destinations, and
  evidence-gap rows out of this direct register-to-register rule.
- The CTest backend subset is the canonical proof log, but focused direct
  `c4cll --codegen obj --target riscv64-linux-gnu` probes were needed to
  observe row movement because the existing per-case logs were not the
  acceptance artifact for this packet.

## Proof

Proof command run for this code packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed. Backend subset used: `^backend_`. Log path:
`test_after.log`. The proof is sufficient for this Step 2 consumer slice.
