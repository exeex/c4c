# Explicit GOT Materialization Policy

Status: Closed
Created: 2026-05-15

Parent Context: ideas/open/233_aarch64_global_address_materialization.md

## Goal

Add an explicit compiler-owned policy source that can mark a global address as
GOT-required before AArch64 address-materialization selection.

## Why This Exists

The AArch64 global-address materialization runbook reached GOT-backed globals
and found no authoritative policy input. `PreparedAddressMaterializationKind::
GotGlobal` exists as a placeholder, but no TargetProfile, BIR global, prepared
state, or AArch64 selector fact currently says that a particular address-
producing global must use GOT materialization.

Without this prerequisite, GOT selection would have to infer policy from symbol
spelling, `is_extern`, or downstream assembler relocation names. Those sources
are not semantic policy and must not drive prepared/MIR selection.

## In Scope

- Identify the narrowest existing layer that should own GOT-required policy
  for address-producing globals.
- Add structured policy facts for relocation model, global preemptibility, or
  another explicit decision source sufficient to distinguish GOT-required
  globals from direct page+low12 globals.
- Carry the chosen policy into prepared address materialization records without
  changing direct global or label behavior.
- Expose diagnostics when GOT policy is unavailable or incomplete.
- Add focused tests or dumps proving a global can be marked GOT-required
  without relying on symbol text or `is_extern` alone.

## Out Of Scope

- Do not implement terminal GOT assembly printing as part of the policy source
  unless the active runbook explicitly delegates it after policy exists.
- Do not infer GOT from symbol spelling, `is_extern` alone, or external
  assembler relocation enum availability.
- Do not design a full platform ABI matrix beyond the minimum explicit policy
  needed to unblock AArch64 GOT address materialization.
- Do not rewrite direct page+low12 global or label materialization except for
  shared carrier compatibility.

## Acceptance Criteria

- A real source policy can mark an address-producing global as GOT-required.
- Prepared address materialization can publish `GotGlobal` only from that
  explicit policy.
- Missing or unsupported policy states fail with diagnostics that name the
  absent policy input.
- The original AArch64 address-materialization runbook can resume Step 6
  without relying on forbidden inference.

## Reviewer Reject Signals

- Reject changes that classify GOT-required globals from symbol spelling,
  `is_extern` alone, test fixture names, or downstream assembler relocation
  enum names.
- Reject expectation rewrites or diagnostic-only changes claimed as GOT policy
  progress when no structured policy reaches prepared address materialization.
- Reject broad relocation-model rewrites that are not needed to let AArch64
  address materialization distinguish direct from GOT-backed globals.
- Reject any route that weakens direct global, string constant, or label
  address materialization contracts to make GOT cases pass.

## Completion Notes

- Completed by commit `8915656eb` and its preceding implementation commits.
- Explicit GOT policy now reaches target options, BIR symbol metadata, prepared
  address-materialization facts, and AArch64 selected `GotPageLow12` records.
- The prerequisite is complete because idea 233 can resume without inferring
  GOT from symbol spelling, `is_extern`, fixture names, or downstream
  relocation enum availability.
- Terminal GOT assembly printing remains out of scope for this prerequisite
  and belongs to the resumed address-materialization runbook.
