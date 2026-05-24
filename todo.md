Status: Active
Source Idea Path: ideas/open/prealloc-runtime-carrier-naming-alignment.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Runtime And Carrier Terminology

# Current Packet

## Just Finished

Completed Step 1 no-code inventory for runtime-helper and special-carrier
terminology.

Terminology map:

- `runtime_helpers.hpp` owns helper-call facts for i128 and f128 lowering:
  helper family/kind, callee name, result ownership, ABI transition,
  marshaling direction, ABI register bindings, call resource policy, live
  preservation policy, clobbered registers, selected call ownership, and missing
  helper facts.
- `special_carriers.hpp` owns prepared value-carrier facts: i128 and f128 value
  storage carriers, atomic operation facts, intrinsic facts, and inline-asm
  facts. These records describe whether later prealloc phases have enough
  value/home/operation metadata; they are not helper-call resource plans.
- `i128_runtime_helpers.cpp` and `f128_runtime_helpers.cpp` consume the i128/f128
  special carriers to build helper ABI lanes, scalar ownership, marshaling, and
  call ownership. Their `CarrierBinding` and `LaneBinding` fields correctly
  refer to source value carriers used by helper calls.
- `atomics.cpp`, `intrinsics.cpp`, and `inline_asm.cpp` use `*Carrier` as a
  generic complete/missing fact bundle. Intrinsics additionally records
  `source_callee_name` and `has_prepared_call_plan`, so it is the one special
  carrier that visibly references call-plan facts without becoming a runtime
  helper resource contract.

Potentially confusing names or comments:

- `ResourcePolicy` in both runtime-helper structs prints as `resources=[...]`;
  this is helper-call resource policy, but the name is generic beside special
  carrier facts.
- `SelectedCallOwnershipPolicy` mixes ownership language with booleans named
  `has_resource_policy`, `has_clobber_policy`, and `has_marshaling`; it is a
  helper-call completeness summary, not ownership of carrier facts.
- Runtime-helper nested `CarrierBinding` / `LaneBinding` are appropriate because
  they bind source value carriers, but they can read too close to the separate
  top-level special carrier contract unless comments clarify the boundary.
- Special-carrier families for atomics, intrinsics, and inline asm use
  `carrier_kind = Complete/Missing` even when the printed dump intentionally
  uses domain labels such as `atomic_operation`, `intrinsic_carrier`, and
  `inline_asm_carrier`.

Names that should stay unchanged for now:

- Prepared dump section headers and record labels:
  `prepared-i128-runtime-helpers`, `prepared-f128-runtime-helpers`,
  `i128_helper`, `f128_helper`, `prepared-i128-carriers`,
  `prepared-f128-carriers`, `i128_carrier`, `f128_carrier`,
  `prepared-atomic-operations`, `atomic_operation`,
  `prepared-intrinsic-carriers`, `intrinsic_carrier`,
  `prepared-inline-asm-carriers`, and `inline_asm_carrier`.
- Existing printed field names including `result_ownership`, `resources`,
  `abi_transition`, `carriers`, `abi_bindings`, `marshaling`,
  `live_preservation`, `selected_call_ownership`, `missing_facts`,
  `source_callee`, and `prepared_call_plan`; changing them would churn
  established prepared-dump vocabulary without a semantic contract rename.
- `Prepared*Carrier` aggregate type names should not be renamed in the first
  code packet because they span public prepared-module contracts and multiple
  printers.

Printer mirror notes:

- `prepared_printer/runtime_helpers.cpp` mirrors runtime-helper contracts and
  already separates helper resources from carrier facts by printing helper
  resources as `resources=[...]` and source carrier bindings under `carriers`.
- `prepared_printer/special_carriers.cpp` mirrors i128/f128 value carrier facts
  only; it does not print helper-call resources.
- `prepared_printer/atomics.cpp` intentionally prints `atomic_operation` rather
  than `atomic_carrier`, which is clearer than the backing aggregate name.
- `prepared_printer/intrinsics.cpp` and `prepared_printer/inline_asm.cpp` retain
  `intrinsic_carrier` and `inline_asm_carrier`; those are established fact
  bundle labels and should move only with an explicit public data rename.

## Suggested Next

Execute Step 2 with one small comment/grouping repair in
`src/backend/prealloc/runtime_helpers.hpp`: add short comments around the
runtime-helper `ResourcePolicy`, `SelectedCallOwnershipPolicy`, and nested
carrier/lane binding structs clarifying that these are helper-call resource and
ABI marshal contracts, while top-level special carriers remain value/fact
contracts. Leave public type names and prepared-printer labels unchanged.

## Watchouts

- Keep runtime helper-call resources separate from special carrier facts.
- Do not change helper selection, marshal behavior, carrier requirements,
  intrinsic support decisions, atomics, inline asm, or dump meaning.
- Avoid renaming prepared dump labels in Step 2; printer alignment should wait
  until a public contract rename actually happens.
- Do not treat atomics, intrinsics, or inline asm as runtime helpers. Their
  `carrier_kind` fields are fact-completeness flags, not helper-call resource
  policies.
- `intrinsic_carriers` references `prepared_call_plan` and `source_callee`
  because intrinsic validation depends on existing call-plan facts; this should
  stay separate from runtime-helper callee/resource ownership.

## Proof

No-code inventory proof: `git diff --check` passed. No `test_after.log` was
created for this packet.
