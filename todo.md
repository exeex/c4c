Status: Active
Source Idea Path: ideas/open/247_explicit_got_materialization_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Policy Owner

# Current Packet

## Just Finished

Step 1 located the narrowest explicit owner for GOT-required global address
policy.

Decision:

- The owner is combined target-wide plus per-symbol policy, not symbol spelling
  and not `is_extern` alone.
- `TargetProfile` should own target-wide relocation mode because `-fPIC`,
  `-fpic`, `-fPIE`, and `-fpie` describe compilation mode, not a property of
  any one global. Add an explicit relocation-mode field there, for example
  `TargetRelocationModel relocation_model`, with values such as `Static`,
  `Pic`, and `Pie`.
- BIR global/function symbol metadata should own the resolved per-symbol
  materialization policy because GOT is required only for some symbols under
  some target modes. Add an explicit policy field for address materialization,
  for example `GlobalAddressMaterializationPolicy address_materialization_policy`
  with `Unspecified`, `Direct`, and `GotRequired`.
- Prepared address materialization should then consume the resolved BIR policy:
  `GotRequired` produces `PreparedAddressMaterializationKind::GotGlobal`,
  `Direct` produces `DirectGlobal` or `TlsGlobal` as today, and `Unspecified`
  in a mode where policy is needed diagnoses instead of silently defaulting.

Why combined:

- Target-wide PIC/PIE is necessary but not sufficient; same-module hidden or
  non-preemptible data can still be direct in many models.
- Per-global `is_extern` is necessary context but not sufficient; it does not
  encode PIC/PIE, visibility, import/preemptibility, or target relocation
  rules.
- AArch64 selection should not infer GOT from rendered names or assembler
  relocation spellings; it should receive an already explicit prepared
  `GotGlobal` carrier with symbol identity and result-home facts.

Unsupported dimensions to diagnose, not default:

- PIC/PIE mode without resolved per-symbol preemptibility/materialization
  policy.
- `GotRequired` without structured `LinkNameId` symbol identity.
- `GotRequired` for TLS symbols until TLS/GOT interaction is specified.
- Unsupported relocation model values or target OS/ABI combinations that have
  no AArch64 GOT policy implementation yet.

## Suggested Next

Step 2 implementation packet: add the explicit policy carriers without
selecting or printing GOT yet.

Suggested packet:

`to_subagent: c4c-executor`
`Objective: Add explicit GOT-required address-materialization policy carriers from target options into BIR/prepared facts, without AArch64 GOT selection or printer output.`
`Plan Step: Step 2: Add Explicit GOT Policy Carriers`
`Owned Files: todo.md, src/target_profile.hpp, src/target_profile.cpp, src/apps/c4cll.cpp, src/backend/bir/bir.hpp, src/backend/bir/lir_to_bir/globals.cpp, src/backend/prealloc/prealloc.hpp, src/backend/prealloc/prepared_printer.cpp, src/backend/prealloc/stack_layout/coordinator.cpp, focused backend BIR/prepared tests`
`Do Not Touch: plan.md, ideas/open/247_explicit_got_materialization_policy.md, src/backend/mir/aarch64/codegen/machine_printer.cpp, unrelated frontend/x86 files`
`Proof: (cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
`Done When: TargetProfile carries relocation mode from -fPIC/-fPIE inputs, BIR global/function metadata can carry explicit Direct/GotRequired/Unspecified address-materialization policy, prepared address materialization preserves that policy and diagnoses missing/unsupported policy instead of inferring from symbol spelling or is_extern alone.`

## Watchouts

- `TargetProfile` currently has no relocation-model field; CLI PIC/PIE flags
  currently define preprocessor macros only.
- BIR globals currently carry `is_extern`, `is_thread_local`, `is_constant`,
  link-name, size/alignment, and initializer facts, but not visibility or
  preemptibility.
- If Step 2 cannot resolve per-symbol policy honestly, it should add diagnostics
  and leave `GotGlobal` unproduced rather than treating all externs as GOT.
- Do not reopen direct page+low12 global/string-constant, label selection, or
  terminal printer behavior while adding policy carriers.

## Proof

Inspection-only packet. No build/tests were run, and no `test_after.log` was
created or modified.
