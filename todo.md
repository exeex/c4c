Status: Active
Source Idea Path: ideas/open/625_prepared_stack_slot_preservation_source_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish prepared preserve source facts

# Current Packet

## Just Finished

Completed Step 3 producer publication for prepared stack-slot preserve source
facts. In `src/backend/prealloc/call_plans.cpp`,
`build_call_preserved_values(...)` now asks the existing
`make_preservation_value_source_endpoint(...)` helper to prefer a prepared
register value-home for StackSlot preserves only when that home proves a
concrete register name. The helper also copies the value-home
`target_register_identity` into `PreparedCallBoundaryEffectEndpoint` for that
register source. No RV64 consumer inference, ABI-index guessing, testcase-name
matching, final layout lookup, or parallel carrier was added.

The existing stack-slot source behavior is preserved: rows whose source is a
real stack slot still publish `preservation_source=stack_slot:...` and keep
`preservation_reason=stack_slot_preservation`.

Updated
`tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
`check_stack_cross_call_preservation_contract()` to require a producer-level
RV64 stack-slot preserved value with:

- `preservation_source` encoded as a concrete register endpoint with value
  identity, register name, GPR bank, register units, and target register
  identity.
- `preservation_destination` encoded as a stack-slot endpoint with slot id,
  stack offset, size, and alignment.
- no callee-saved preserve metadata on the StackSlot row.

Diagnostic evidence from the delegated proof shows the expected producer rows:

- `20020529-1.c`: `%p.b` publishes `preservation_source=register:a1:value#1`
  with a stack-slot destination.
- `20000412-4.c`: `%p.j` publishes `register:a1`; `%p.width` publishes
  `register:a3`.
- `pr51933.c`: `%p.y` publishes `register:a1`.
- `pr51933.c` also keeps `@.str0` as
  `preservation_source=stack_slot:slot#1100:value#2199`, confirming the
  stack-slot source path remains separate and fail-closed.

## Suggested Next

Execute Step 4 from `plan.md`: wire RV64 preserve guards to require the
prepared register source endpoint and stack-slot destination facts without
manufacturing missing source authority in RV64.

## Watchouts

- For ABI-home register sources, the prepared value-home proves
  `target_register_identity`; it may not prove a regalloc pool
  `register_placement`. Do not make RV64 infer missing placement from ABI
  index.
- Step 4 should consume the prepared `preservation_source` and
  `preservation_destination` facts as authority and fail closed when either is
  absent or contradictory.
- Keep `preservation_source=stack_slot:...` rows such as `pr51933.c` `@.str0`
  out of the caller-saved register-source route.
- Residuals remain outside this producer packet: `20000412-4.c` downstream
  move-bundle ambiguity, `20020529-1.c` ordinary call/result ABI residual, and
  `pr51933.c` inline asm policy.

## Proof

Ran exactly the supervisor-delegated Step 3 proof command from
`/workspaces/c4c`:

```bash
bash -lc 'set -euo pipefail
cmake --build build --target backend_prepare_frame_stack_call_contract_test c4cll > test_after.log
ctest --test-dir build -j --output-on-failure -R "^(backend_prepare_frame_stack_call_contract)$" >> test_after.log
out=build/agent_state/625_step3_preserve_source_facts
rm -rf "$out"
mkdir -p "$out"
for src in tests/c/external/gcc_torture/src/20020529-1.c tests/c/external/gcc_torture/src/20000412-4.c tests/c/external/gcc_torture/src/pr51933.c; do
  name=$(basename "$src" .c)
  ./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$out/${name}.prepared.txt" 2> "$out/${name}.prepared.err" || true
  rg -n "preserve value=.*route=stack_slot|preservation_source=register:|preservation_source=register:value|preservation_destination=stack_slot|caller_saved_clobber_reuse_stack_preservation" "$out/${name}.prepared.txt" > "$out/${name}.evidence.txt" || true
done
{
  echo "# idea 625 step 3 diagnostic evidence"
  rg -n "preservation_source=register:(a1|a3)|preservation_source=register:value|caller_saved_clobber_reuse_stack_preservation|preservation_destination=stack_slot" "$out"/*.evidence.txt || true
} > "$out/summary.txt"
cat "$out/summary.txt" >> test_after.log
'
```

Result: build passed, `backend_prepare_frame_stack_call_contract` passed
1/1, and diagnostic artifacts are under
`build/agent_state/625_step3_preserve_source_facts/`. Canonical proof log:
`test_after.log`.
