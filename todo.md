Status: Active
Source Idea Path: ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Existing Prepared Fact Failures

# Current Packet

## Just Finished

Step 1 - Inventory Existing Prepared Fact Failures: inventoried existing
prepared-fact failure/status surfaces and selected the first verifier slice
families.

Selected first verifier slice:

- Value home / typed storage authority
  - Current surfaces:
    `src/backend/prealloc/decoded_home_storage.hpp:51` defines
    `PreparedDecodedHomeStorageStatus` values for `MissingAuthority`,
    `MissingRegisterPlacement`, missing payload/slot/name states,
    `UnsupportedStorageEncoding`, and `UnsupportedValueHomeKind`.
    `src/backend/prealloc/decoded_home_storage.hpp:108` defines diagnostic
    categories for missing value authority, missing typed register authority,
    unsupported storage-plan authority, and unsupported value-home authority.
    `src/backend/prealloc/decoded_home_storage.cpp:241` builds the current
    diagnostic messages, including "no typed prepared authority exists for
    value operand", "value-home register spelling is diagnostic-only until
    typed placement exists", and unsupported typed operand form messages.
    AArch64 maps these shared categories into target diagnostics at
    `src/backend/mir/aarch64/codegen/operands.cpp:162`.
    Tests cover the diagnostic builder at
    `tests/backend/bir/backend_prealloc_decoded_home_storage_test.cpp:386`
    and AArch64 operand-resolution diagnostics at
    `tests/backend/mir/backend_aarch64_operand_resolution_test.cpp:186`.
  - Initial verifier diagnostic owners:
    `MissingAuthority` and `MissingRegisterPlacement` should classify as
    `producer-missing` unless the prepared record is present but contradictory,
    then `producer-incoherent`; `UnsupportedStorageEncoding` and
    `UnsupportedValueHomeKind` should classify as
    `target-unsupported-but-coherent` when all required producer facts are
    present.

- Call boundary argument/result plan authority
  - Current surfaces:
    `src/backend/prealloc/calls.hpp:644` defines
    `PreparedCallBoundaryMoveClassificationStatus` values for
    `UnsupportedOpKind`, `MissingAbiIndex`, `MissingCallArgumentPlan`,
    `MissingCallResultPlan`, `MismatchedCallResultPlan`, and
    `MissingAbiBinding`. `src/backend/prealloc/call_plans.cpp:3293`
    classifies each call-boundary move and preserves matched argument/result
    authority when a binding is missing or mismatched.
    `src/backend/prealloc/call_plans.cpp:3500` carries that classification into
    `PreparedCallBoundaryEffectPlan::classification_status` for consumers.
    Tests cover missing/mismatched classification at
    `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp:380`
    and an x86 consumer rejecting unavailable explicit effects at
    `tests/backend/mir/backend_x86_call_boundary_effect_ordering_test.cpp:221`.
  - Initial verifier diagnostic owners:
    `MissingAbiIndex`, `MissingCallArgumentPlan`, `MissingCallResultPlan`, and
    `MissingAbiBinding` should classify as `producer-missing`;
    `MismatchedCallResultPlan` should classify as `producer-incoherent`;
    `UnsupportedOpKind` should classify as `target-unsupported-but-coherent`
    when the prepared call facts are otherwise complete.

- Variadic entry helper operand homes / access plans
  - Current surfaces:
    `src/backend/prealloc/variadic.hpp:306` defines
    `PreparedVariadicEntryHelperOperandHomes`, and
    `src/backend/prealloc/variadic.hpp:321` defines completeness predicates for
    `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy` helper
    operands/access plans. `src/backend/mir/aarch64/codegen/variadic.cpp:62`
    reports missing consumption facts such as `helper_operand_homes`, while
    `src/backend/mir/aarch64/codegen/variadic.cpp:1223` exposes
    `variadic_helper_operand_homes_complete` and
    `src/backend/mir/aarch64/codegen/variadic.cpp:1228` maps helper-specific
    missing access plans into current AArch64 diagnostics.
    Tests cover missing entry-plan and missing consumption facts at
    `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp:14619`
    and scalar/aggregate missing access plans at
    `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp:15255`
    and `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp:15405`.
  - Initial verifier diagnostic owners:
    missing `helper_operand_homes`, missing scalar/aggregate access plans, and
    incomplete helper homes should classify as `producer-missing` or
    `producer-incoherent` depending on whether the helper record is absent or
    present-but-incomplete; unsupported target ABI/helper-layout facts should
    stay separate as `target-unsupported-but-coherent`.

Not selected for the first slice but observed as a near follow-up:

- Edge/block publication facts already expose missing and unsupported statuses
  through `src/backend/mir/riscv/codegen/emit.hpp:27` and RV64 intent
  classification in
  `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp:462`.
  This family should wait until the first verifier surface exists because the
  status space spans publication lookup, source-home support, and destination
  home support.

## Suggested Next

Proceed to Step 2: add the initial verifier/report data model and invocation
point for the three selected families above, preserving the existing status
vocabulary as evidence for owner classification.

## Watchouts

Do not change tests, unsupported markers, gcc_torture allowlists, or target
fallback inference to claim progress. Missing producer facts should route to
prepared verification or producer diagnostics before target consumers run.
Keep publication facts out of the first implementation slice unless the
supervisor explicitly broadens scope; they are related but have enough status
surface to deserve a separate verifier row once the report API exists.

## Proof

Inventory-only packet; no build or test command required by the supervisor.
Evidence was gathered with `rg` plus focused reads of the referenced files.
No `test_after.log` was produced because the delegated proof explicitly
requested grep/file-reference evidence in `todo.md` instead of a command.
