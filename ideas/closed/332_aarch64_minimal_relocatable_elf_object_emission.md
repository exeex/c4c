# AArch64 Minimal Relocatable ELF Object Emission

## Closure Note

Closed after the AArch64 child added target-owned encoded fragments, typed
fixups for `Call26`, `AdrPrelPgHi21`, and `AddAbsLo12Nc`, structured machine
record object emission for the minimal return/direct-call subset, and
relocatable ELF serialization checks for AArch64 `.text`, symbols, and
relocations. The close proof preserved nearby AArch64 asm-route coverage and
passed the focused 8-test object/asm subset with the regression guard in
non-decreasing mode.

Remaining rollout work is intentionally covered by separate open children:
`333_codegen_obj_cli_and_test_integration.md`,
`334_object_route_scan_and_default_readiness.md`, and
`335_textual_assembler_object_route_followup.md`. This child did not expose
`--codegen obj`, switch defaults, run c-testsuite object-route scans, or add
broader data/global/BSS, branch, GOT/TLS, indirect/variadic, stack-heavy, or
runtime object-route coverage.

## Goal

Add the first direct AArch64 relocatable ELF object-emission path for the
current supported backend subset, using the shared native object model and
AArch64-owned relocation/fixup rules.

## Why This Exists

The native object route must support both RV64 and AArch64. AArch64 should
share sections, symbols, labels, relocations, and ELF serialization with RV64,
but its instruction encodings, relocation kinds, ADRP/ADD pairing, branch
fixups, alignment behavior, and target flags must remain target-owned.

## In Scope

- Add AArch64 target fragment or encoder records that carry encoded
  instruction bytes plus typed AArch64 fixups.
- Integrate AArch64 function and data emission with the shared object model.
- Support the minimal relocation set needed for current AArch64 backend output,
  including calls, branches, symbol references, and ADRP/ADD-style address
  materialization when present in the supported subset.
- Add object contract and link/runtime smoke tests for AArch64 `.o` output
  beside existing asm-route coverage.
- Keep AArch64 target ELF machine flags and relocation number mapping
  target-local.

## Out Of Scope

- RV64 object emission or RV64 relocation policy.
- CLI default switching or c-testsuite-wide object-route scans.
- GNU-compatible textual assembly parsing.
- Replacing the current AArch64 `.s` route.
- Broad AArch64 backend semantic fixes unrelated to object emission.

## Acceptance And Proof Expectations

- Focused AArch64 object tests can inspect `.o` structure with readelf or
  llvm-objdump and link the produced object into a runnable binary where the
  local environment supports the target proof.
- AArch64 object emission uses backend machine records and the shared object
  model, not assembly text round-tripping.
- AArch64 relocation tests prove at least one call or branch relocation and one
  symbol-address relocation needed by current backend output.
- The equivalent asm-route tests still pass.

## Dependency Notes

- Depends on the shared object model/API child:
  `ideas/open/330_native_object_model_and_emission_api.md`.
- Should follow or run in parallel with RV64 target work only after the shared
  API is stable enough to avoid target-specific forks.
- CLI/test integration should not advertise AArch64 object support until this
  child has accepted proof for the minimal subset.

## Reviewer Reject Signals

- AArch64 `.o` emission is implemented by printing `.s` and parsing or
  externally assembling it on the compiler's primary object path.
- RV64 relocation concepts such as `pcrel_hi` leak into shared or AArch64 API
  names instead of using AArch64-owned fixup kinds.
- Relocation kinds are represented as loose strings or raw numbers throughout
  the AArch64 backend instead of typed target fixups.
- The slice passes only one named AArch64 smoke case through testcase-shaped
  matching while nearby calls, labels, or symbol references remain unexamined.
- Existing AArch64 asm-route tests or expectations are downgraded to hide
  object route gaps.
- The change expands into RV64, CLI default switching, or c-testsuite-wide
  policy instead of staying on minimal AArch64 object emission.
- The object file is structurally present but not linkable or inspectable for
  the claimed supported subset.
