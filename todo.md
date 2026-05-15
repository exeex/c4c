Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Populate GOT-Backed Global Materialization

# Current Packet

## Just Finished

Step 6 is blocked before implementation: there is no real backend-owned
GOT-required global address policy input to consume.

Findings:

- `PreparedAddressMaterializationKind::GotGlobal` exists as a placeholder, but
  no prepared producer currently classifies a real global address as GOT-backed.
- `TargetProfile` carries target triple, arch, OS, ABI, and float-register
  capabilities only; it has no PIC/PIE, relocation-model, code-model, or
  preemptibility policy field.
- The CLI accepts `-fPIC/-fpic/-fPIE/-fpie`, but those flags currently define
  preprocessor macros only and are not carried into BIR, prepared state, or
  AArch64 selection policy.
- BIR globals expose `is_extern`, `is_thread_local`, `is_constant`, link-name
  identity, size/alignment, and initializer facts, but do not expose visibility,
  interposition/preemptibility, import model, or a target relocation policy that
  would distinguish direct page+low12 from GOT materialization.
- AArch64 GOT relocation spellings exist only in the external assembler encoder
  compatibility layer (`AdrGotPage21`, `Ld64GotLo12`); that is downstream text
  parsing support, not a structured source policy for prepared/MIR selection.

## Suggested Next

Next packet should add an explicit GOT policy source before trying selection:
for example a target/prepared relocation-model or global-preemptibility field
that says a specific global address must use GOT materialization, plus the
relocation identity facts AArch64 should preserve. After that, Step 6 can be
re-run to populate and select `GotGlobal` records.

## Watchouts

Do not infer GOT from symbol spelling, `is_extern` alone, or the presence of
external-assembler relocation enum names. The missing source is an explicit
frontend/target/prepared policy that marks an address-producing global as
GOT-required and carries the relocation identity to preserve.

## Proof

No implementation proof was run for Step 6 because the packet is blocked before
code changes by the missing GOT policy source described above. No Step 6
`test_after.log` was produced.
