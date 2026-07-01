Status: Active
Source Idea Path: ideas/open/500_semantic_global_static_gep_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route The First Global/Static GEP Packet

# Current Packet

## Just Finished

Step 3 implemented the prerequisite `global_static_gep_authority` certificate
surface during global GEP lowering. `GlobalStaticGepAuthorityRecord` now carries
global/static identity, derivation kind, layout path text, element byte range,
constant or dynamic range facts, range verdict, and LIR producer coordinate
identity. `lower_memory_gep_inst` publishes records from successful direct
global, relative global-pointer, dynamic global pointer-array, dynamic global
aggregate-array, and dynamic global scalar-array GEP branches. Pointer-valued
global GEPs remain boundary records instead of available semantic admission.

Focused coverage in `backend_lir_to_bir_notes` verifies an explicit constant
global array element publishes one `Available` authority record with LinkNameId,
byte range, range verdict, and LIR coordinate, verifies a default-constructed
authority record fails closed instead of defaulting to `Available`, and verifies
raw/no-id dynamic global GEP lowering publishes only fail-closed records rather
than inventing available authority.

Evidence summary:
`build/agent_state/500_step3_global_static_gep_authority_implementation/summary.md`.

## Suggested Next

Ask the plan owner to decide residual lifecycle disposition for idea 500 now
that Step 3 has published production `global_static_gep_authority` records. The
decision should either close or extend the current 500 lifecycle, or activate a
focused final semantic global/static GEP admission consumer idea/plan that
consumes matching available authority records. Any follow-up consumer route
should keep pointer/string, runtime/string intrinsic, aggregate/member,
raw-shape-only, target-only, and coordinate-confusion rows fail-closed unless a
matching available authority and consumer-specific prerequisite exists.

## Watchouts

- This packet intentionally does not implement RV64/MIR lowering or final
  semantic global/static GEP admission.
- `src/20051104-1.c` remains a string/global-pointer provenance boundary:
  a global pointer field GEP record is not sufficient to admit the pointed
  string-literal subobject behind `s.name[s.len]`.
- `src/ieee/copysign2.c` remains a runtime/string intrinsic consumer boundary
  until a later packet proves the consumer-specific authority.
- Do not reconstruct this certificate later from prepared object data, final
  homes, relocations, raw testcase shape, or target behavior; the certificate is
  captured while LIR global GEP lowering has producer coordinates.

## Proof

Step 3 implementation proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed after fail-closed default repair. Output preserved in
`test_after.log`.
