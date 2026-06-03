Status: Active
Source Idea Path: ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Global Identity Proof

# Current Packet

## Just Finished

Step 4 focused proof completed for the raw global identity fallback contract.

Updated `tests/backend/bir/backend_prepare_stack_layout_test.cpp`:
- Strengthened `prepare_link_name_authoritative_global_access_module` /
  `check_link_name_authoritative_global_access_activation`.
- Ordinary global load/store coverage:
  - ID-only `LoadGlobalInst::global_name_id` still publishes a prepared
    `GlobalSymbol` access for `g.authoritative`.
  - Drifted raw spelling paired with the structured ID still fails closed.
  - Raw/no-id ordinary load using raw name `g.authoritative` now proves missing
    `global_name_id` cannot resolve a structured module global.
- Raw/no-id compatibility coverage:
  - The retained compatibility path is proved only with raw/no-id global
    `g.compat`, including the existing ordinary store and a new explicit
    `MemoryAddress::GlobalSymbol` load.
- Explicit/GEP-derived `MemoryAddress::GlobalSymbol` coverage:
  - A `base_link_name_id` address publishes structured symbol facts and combines
    instruction/address offsets.
  - Raw `base_name` alone fails closed for structured global `g.authoritative`.
  - Raw/no-id `base_name` works only for raw/no-id global `g.compat`.
- Direct/GOT/TLS materialization coverage:
  - Existing direct, GOT, TLS, and indirect-callee materialization assertions
    still prove structured `pointer_symbol_link_name_id` materialization.
  - Added a raw `@g.raw.materialization` pointer value without
    `pointer_symbol_link_name_id` and asserted it produces no global
    materialization.
- Initializer-derived coverage:
  - Existing included `backend_lir_to_bir_notes` assertions prove lowered scalar
    and aggregate pointer initializers carry structured pointer-symbol IDs and
    reject missing-ID fallbacks; this packet kept those expectations intact and
    exercised them in the delegated proof command.

## Suggested Next

Execute `plan.md` Step 5: run final validation and record close-readiness for
ordinary global loads/stores, GEP-derived/global-address routes, direct global
materialization, and initializer-derived pointer-symbol identity.

## Watchouts

- Do not make raw symbol spelling the primary identity path when structured
  `LinkNameId` exists.
- Keep final target relocation selection and TLS lowering out of scope.
- The delegated CTest regex matched four tests in this build:
  `backend_aarch64_prepared_memory_operand_records`, `backend_lir_to_bir_notes`,
  `backend_prepare_stack_layout`, and `backend_prepared_printer`.
- No source changes were made in this Step 4 packet.

## Proof

Passed. Ran:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "backend_prepare_stack_layout|backend_aarch64_prepared_memory_operand_records|backend_lir_to_bir_notes|backend_x86_handoff_boundary_i32_guard_chain|backend_prepared_printer" --output-on-failure; } 2>&1 | tee test_after.log`

Proof log: `test_after.log`.
