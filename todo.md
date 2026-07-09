Status: Active
Source Idea Path: ideas/open/625_prepared_stack_slot_preservation_source_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove breadth and close-readiness

# Current Packet

## Just Finished

Completed Step 5 breadth and close-readiness proof without implementation
edits. The Step 5 diagnostic refresh shows explicit prepared source endpoint
facts for the shared ordinary stack-slot preserve family:

- `20020529-1.c`: `%p.b` now publishes
  `preservation_source=register:a1:value#1` and
  `preservation_destination=stack_slot:slot#19:value#1`.
- `20000412-4.c`: `%p.j` now publishes
  `preservation_source=register:a1:value#1`; `%p.width` now publishes
  `preservation_source=register:a3:value#3`.
- `pr51933.c`: `%p.y` now publishes
  `preservation_source=register:a1:value#4`; existing `@.str0` stack-slot
  source rows remain stack-slot sourced.

Before/after owner comparison:

- Step 1 prepared evidence had stack-slot preserves with incomplete
  `preservation_source=register:value#...` facts for the ordinary rows above.
- Step 5 prepared evidence has concrete register identities on the same
  caller-saved stack-slot preserve family.
- Step 1/Step 4 `20000412-4.c` remained owned by
  `rv64_prepared_move_bundle_consumer` /
  `ambiguous_non_parallel_multi_source_stack_destination`; Step 5 confirms the
  same non-preserve residual.
- Step 1 `pr51933.c` and Step 5 both remain owned by
  `unsupported_inline_asm_fragment`, while its preserve-source facts are now
  concrete where register sourced.
- Step 4/Step 5 `20020529-1.c` now reaches
  `unsupported_terminator_fragment`; the missing stack-slot preserve-source
  authority is no longer the first owner for this row.

Residual owner buckets:

- `20020529-1.c`: RV64 terminator lowering
  (`unsupported_terminator_fragment`).
- `20000412-4.c`: RV64 prepared move-bundle authority for non-parallel
  register-source fan-in to one stack destination
  (`rv64_prepared_move_bundle_consumer` /
  `ambiguous_non_parallel_multi_source_stack_destination`).
- `pr51933.c`: inline asm carrier policy (`unsupported_inline_asm_fragment`).

Negative guard status:

- Step 4 guard tests for missing source target identity, non-register source,
  stale preserved-call coordinates, missing destination stack offset, and
  contradictory destination offset remain covered by
  `backend_riscv_object_emission`.
- Step 5 broad backend proof reran the touched backend surfaces and kept those
  guards green.

Recommendation: idea 625 is close-ready. The source-authority route repaired
the prepared preserve facts for a shared ordinary stack-slot preserve family;
remaining object failures belong to separate terminator, move-bundle, and
inline-asm owners and should be split or handled under later source ideas.

## Suggested Next

Supervisor should hand this active plan to the plan owner for close/deactivate
review. If the source idea is closed, keep residual work split by owner rather
than extending idea 625.

## Watchouts

- Do not broaden closeout into `unsupported_terminator_fragment`,
  `rv64_prepared_move_bundle_consumer`, or `unsupported_inline_asm_fragment`.
  These are residual owner buckets, not preserve-source authority failures.
- The proof did not use expectation, unsupported-marker, allowlist, timeout,
  runtime, or accounting-only changes as progress.

## Proof

Ran exactly the supervisor-delegated Step 5 proof command from
`/workspaces/c4c`:

```bash
bash -lc 'set -euo pipefail
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test > test_after.log
ctest --test-dir build -j --output-on-failure -R "^backend_" >> test_after.log
out=build/agent_state/625_step5_breadth_close_readiness
rm -rf "$out"
mkdir -p "$out"
for src in tests/c/external/gcc_torture/src/20020529-1.c tests/c/external/gcc_torture/src/20000412-4.c tests/c/external/gcc_torture/src/pr51933.c; do
  name=$(basename "$src" .c)
  ./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$out/${name}.prepared.txt" 2> "$out/${name}.prepared.err" || true
  rg -n "preserve value=.*route=stack_slot|preservation_source=register:|preservation_source=register:value|preservation_source=stack_slot|preservation_destination=stack_slot|caller_saved_clobber_reuse_stack_preservation" "$out/${name}.prepared.txt" > "$out/${name}.prepared_evidence.txt" || true
  set +e
  ./build/c4cll --codegen obj --target riscv64-linux-gnu "$src" -o "$out/${name}.bin" > "$out/${name}.obj.out" 2> "$out/${name}.obj.err"
  echo $? > "$out/${name}.obj.rc"
  set -e
  rg -n "unsupported_call_abi|unsupported_terminator_fragment|rv64_prepared_move_bundle_consumer|ambiguous_non_parallel_multi_source_stack_destination|unsupported_inline_asm_fragment|preserv|source|destination|stack" "$out/${name}.obj.err" "$out/${name}.obj.out" > "$out/${name}.obj_evidence.txt" || true
done
{
  echo "# idea 625 step 5 breadth and close-readiness"
  echo "## prepared source facts"
  rg -n "preservation_source=register:(a1|a3)|preservation_source=register:value|preservation_source=stack_slot|caller_saved_clobber_reuse_stack_preservation" "$out"/*.prepared_evidence.txt || true
  echo "## object residuals"
  for rc in "$out"/*.obj.rc; do echo "$(basename "${rc%.obj.rc}") rc=$(cat "$rc")"; done
  rg -n "unsupported_call_abi|unsupported_terminator_fragment|rv64_prepared_move_bundle_consumer|ambiguous_non_parallel_multi_source_stack_destination|unsupported_inline_asm_fragment|preserv|source|destination|stack" "$out"/*.obj_evidence.txt || true
} > "$out/summary.txt"
cat "$out/summary.txt" >> test_after.log
'
```

Result: build passed; `ctest --test-dir build -j --output-on-failure -R
"^backend_"` passed 347/347; diagnostic object probes produced the expected
residual `rc=2` classifications for the three rows. Diagnostic artifacts are
under `build/agent_state/625_step5_breadth_close_readiness/`. Canonical proof
log: `test_after.log`.
