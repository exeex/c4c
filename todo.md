Status: Active
Source Idea Path: ideas/open/609_rv64_global_data_consumer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume prepared object-data symbol publication

# Current Packet

## Just Finished

- Completed Step 2 (`Consume prepared object-data symbol publication`) in
  `src/backend/mir/riscv/codegen/object_emission.cpp`.
- RV64 object emission now consumes prepared selected object-data publication
  for coherent selected records without inventing missing bytes or relocation
  targets: pointer initializers use resolved prepared link identity, repeated
  same-label object-data records can be superseded by later object records, and
  object-data lookup prefers the record matching the current prepared global
  extent.
- Direct representative probes moved `src/pr61517.c`, `src/pr57877.c`,
  `src/pr57860.c`, and `src/20030224-2.c` past the prior
  `prepared selected object-data contract status=unsupported_but_coherent`
  stop and compiled them to RV64 objects.
- `src/20010924-1.c` remains at
  `RV64 object route cannot emit prepared relocation object data without relocation records`,
  preserving it as the relocation-only guard for Step 3.

## Suggested Next

Start Step 3 (`Emit prepared relocation records for object data`) with
`src/20010924-1.c` as the first relocation-only representative. Recheck the
mixed rows after relocation support lands to confirm they remain past Step 2
and do not require target-side reconstruction.

## Watchouts

- Do not produce missing prepared/global authority in this plan.
- Do not infer object bytes, relocation slots, target identity, or access
  widths inside RV64 when prepared facts are absent.
- Keep expectation, unsupported-marker, allowlist, timeout, runtime/link, and
  accounting changes out of the proof.
- Step 2 deliberately does not emit prepared relocation slots; keep
  `src/20010924-1.c` assigned to Step 3 unless refreshed evidence shows a
  different first relocation-record blocker.
- Duplicate same-label object-data rows can appear with different extents; the
  RV64 consumer may supersede earlier same-label object symbols only when the
  existing symbol is an object with compatible binding. Non-object or binding
  conflicts remain fail-closed.
- The exact proof command for Step 3 should remain:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

## Proof

- Delegated Step 2 proof passed:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Result: 346 backend tests passed, 0 failed.
- Proof log path: `test_after.log`.
