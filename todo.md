# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 7
Current Step Title: Retire the Optional-Bag Consumer API

## Just Finished

Step 7 continued the optional-bag consumer API retirement audit. The remaining
test-side uses of the generic `has_complete_prepared_variadic_*` helper
shorthands were replaced with typed payload accessor checks so focused tests no
longer rely on the optional-bag completeness consumer API. The only remaining
generic helper uses in the owned implementation audit are in
`variadic_entry_plans.cpp` producer construction paths that decide whether to
publish/remove missing facts while still filling compatibility storage.

## Suggested Next

Continue Step 7 with a header-level cleanup packet that owns
`src/backend/prealloc/variadic.hpp`: rename, narrow, or document the legacy
optional fields and generic completeness wrappers as producer compatibility
storage, then update producer call sites if a clearer internal name is chosen.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- Migrated target, verifier, and printer consumers use typed helper payload
  accessors; do not reintroduce direct optional-bag consumption there.
- `PreparedVariadicEntryHelperOperandHomes` legacy optional fields and the
  `has_complete_prepared_variadic_*` wrappers are defined in `variadic.hpp`,
  which was not owned by this packet; they remain as producer compatibility API
  until a header-owned cleanup can quarantine or rename them.
- Existing producer behavior still writes legacy optional fields while typed
  payloads derive from complete coherent rows for compatibility.
- Aggregate `va_arg` has an RV64-only `payload_write_address` requirement that
  remains object-route support data on the typed aggregate access plan, not
  generic typed payload completeness.
- Stale typed payloads must not mask missing or incomplete optional facts.
  Complete rewritten legacy rows intentionally refresh the derived typed cache
  for compatibility with existing mutable target fixtures.
- Do not infer helper operand homes from stack layout, BIR shape, or source
  spelling in target lowering.
- Remaining optional-bag reads are producer construction, compatibility
  derivation, or test fixture setup/mutation scaffolding; focused tests should
  prefer typed accessor checks when asserting consumer-visible completeness.

## Proof

Ran the supervisor-selected Step 7 proof:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_prealloc_prepared_contract_verifier|backend_prepared_printer|backend_prepare_frame_stack_call_contract|backend_prepared_object_consumer_contract|backend_riscv_object_emission|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_dump_riscv64_variadic_aggregate_overflow_helper_contract|backend_cli_riscv64_variadic_aggregate_overflow_helper_contract_obj)$' > test_after.log 2>&1`

Result: passed; `test_after.log` contains 11/11 passing tests.
