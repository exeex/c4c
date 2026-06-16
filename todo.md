Status: Active
Source Idea Path: ideas/open/292_reopen_286_288_match_load_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair the admitted local-memory shape

# Current Packet

## Just Finished

Step 3 complete for the original `match` formal pointer-parameter blocker:
formal pointer parameters now seed `PointerAddress` from structured
`LirFunction::signature_params` metadata instead of always using an opaque
`Void` pointee. Pointer-valued loads now carry the next-level structured
pointee facts through `PointerAddress`, and local pointer-slot reloads preserve
those facts after store/reload cycles.

Concrete repaired shape:
- `const char **s` seeds `%p.s` as a pointer to a pointer object and records
  that a loaded pointer from `%p.s` points at `i8`.
- `const char *f` seeds `%p.f` as a pointer to `i8`.
- The delegated 286/288 CLI subset no longer reports latest failing function
  `match`; after this slice it advances to latest failing function `myprintf`.

Files expanded beyond the packet's initial owned list because the existing
formal-param seed alone could not preserve loaded pointer facts through the
route-local memory carrier:
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`

## Suggested Next

Execute the next packet against the downstream `myprintf` load local-memory
failure. Start by tracing the first rejected load in `myprintf`, especially
loads through local pointer slots and AArch64 `va_list` field/cursor state, and
decide whether it is the same metadata-propagation family or a separate
va_list-local-memory admission rule.

## Watchouts

- This slice intentionally does not add named `00204.c`, `match`, or
  `myprintf` logic and does not weaken CLI expectations.
- The rendered call-argument suffix fallback boundary from idea 291 was not
  touched.
- The remaining red proof is downstream of the original `match` blocker but is
  still in `load local-memory semantic family`; do not treat the 286/288 subset
  as restored yet.
- `PointerAddress::loaded_pointer_value_type` and
  `loaded_pointer_type_text` are route-local facts for publishing pointer facts
  after a pointer-valued load. Legacy/no-metadata routes still default to
  `Void`.

## Proof

Delegated proof command:
`cmake --build --preset default && ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure`

Proof log:
`test_after.log`.

Result: build passed; the two delegated CLI tests still fail, but the old
latest function `match` failure is repaired. The current latest failure is
semantic lir_to_bir function `myprintf` in `load local-memory semantic family`.

Supervisor acceptance checks:

- `check_monotonic_regression.py --allow-non-decreasing-passed` on the exact
  delegated before/after CLI logs reported no new failing test names.
- `git diff --check` passed.
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'`
  passed after the patch.
