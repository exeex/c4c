Status: Active
Source Idea Path: ideas/open/625_prepared_stack_slot_preservation_source_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Wire RV64 preserve guards without inference

# Current Packet

## Just Finished

Completed Step 4 RV64 preserve consumer wiring. The RV64 object path now looks
up the exact prior `PreparedCallPreservedValue` named by
`PreparedCallArgumentSourceSelection` and requires the producer-published
`preservation_source` / `preservation_destination` endpoints before emitting
prior-preserved reloads. Register-source StackSlot preservation effects now
store the concrete source register into the prepared stack destination before
the call and republish by loading from that stack destination after the call.

Changed files:

- `src/backend/mir/riscv/codegen/prepared_call_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Focused RV64 object tests now cover:

- successful register-source StackSlot preservation through a later
  prior-preserved argument reload
- missing source target register identity
- non-register / mismatched source endpoint
- stale preserved-call coordinates
- missing stack destination offset
- contradictory stack destination offset

## Suggested Next

Execute Step 5 from `plan.md`: rerun the ordinary-call stack-slot preserve
diagnostic subset, compare first-owner buckets before/after, and decide
close-readiness or split residuals.

## Watchouts

- Do not broaden Step 5 into move-bundle ambiguity, inline asm policy, or
  terminator lowering. Those are residual owner buckets, not preserve-source
  authority work.
- Step 4 evidence shows `20020529-1.c` now reaches
  `unsupported_terminator_fragment`, so the previous missing preserve-source
  blocker is no longer the first owner for that row.
- `20000412-4.c` remains
  `rv64_prepared_move_bundle_consumer` /
  `ambiguous_non_parallel_multi_source_stack_destination`.
- `pr51933.c` remains `unsupported_inline_asm_fragment`.

## Proof

Ran exactly the supervisor-delegated Step 4 proof command from
`/workspaces/c4c`:

```bash
bash -lc 'set -euo pipefail
cmake --build build --target backend_riscv_object_emission_test c4cll > test_after.log
ctest --test-dir build -j --output-on-failure -R "^(backend_riscv_object_emission)$" >> test_after.log
out=build/agent_state/625_step4_rv64_preserve_guards
rm -rf "$out"
mkdir -p "$out"
for src in tests/c/external/gcc_torture/src/20020529-1.c tests/c/external/gcc_torture/src/20000412-4.c tests/c/external/gcc_torture/src/pr51933.c; do
  name=$(basename "$src" .c)
  set +e
  ./build/c4cll --codegen obj --target riscv64-linux-gnu "$src" -o "$out/${name}.bin" > "$out/${name}.out" 2> "$out/${name}.err"
  echo $? > "$out/${name}.rc"
  set -e
  rg -n "unsupported_call_abi|rv64_prepared_move_bundle_consumer|unsupported_inline_asm_fragment|preserv|source|destination|stack" "$out/${name}.err" "$out/${name}.out" > "$out/${name}.evidence.txt" || true
done
{
  echo "# idea 625 step 4 rv64 guard evidence"
  for rc in "$out"/*.rc; do echo "$(basename "${rc%.rc}") rc=$(cat "$rc")"; done
  rg -n "unsupported_call_abi|rv64_prepared_move_bundle_consumer|unsupported_inline_asm_fragment|preserv|source|destination|stack" "$out"/*.evidence.txt || true
} > "$out/summary.txt"
cat "$out/summary.txt" >> test_after.log
'
```

Result: build passed, `backend_riscv_object_emission` passed 1/1, and
diagnostic artifacts are under
`build/agent_state/625_step4_rv64_preserve_guards/`. Canonical proof log:
`test_after.log`.
