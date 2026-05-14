# BIR Semantic Gaps From AArch64 C-Testsuite Scan

Status: Draft
Created: 2026-05-14
Parent Context: ideas/open/230_aarch64_c_testsuite_backend_full_scan.md

## Intent

Classify and repair semantic `lir_to_bir` gaps exposed by the AArch64 backend
c-testsuite scan before target-specific codegen tries to consume missing or
unsupported BIR facts.

These failures appear in the AArch64 backend scan, but the observed failure
mode is frontend-stage semantic lowering into BIR rather than an AArch64
assembler or runtime failure.

## Evidence From The Scan

Observed scan command:

```text
ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_'
```

Bucket count:

- Part of 28 `[FRONTEND_FAIL]` AArch64 lowering / machine-printer /
  `lir_to_bir` failures.

Representative semantic-prepared-BIR cases:

- `src/00140.c`
- `src/00143.c`
- `src/00176.c`
- `src/00185.c`
- `src/00216.c`

Observed failure mode:

- semantic `lir_to_bir` failures outside currently admitted capability buckets.

## Owner Layer

Shared frontend semantic lowering, LIR-to-BIR conversion, BIR capability
classification, and prepared-BIR facts where applicable.

AArch64 codegen should consume the resulting structured BIR/prepared facts only
after this layer admits the construct.

## Scope

- Inspect representative `lir_to_bir` failures and group them by missing
  semantic construct or unsupported BIR capability.
- Decide whether each group belongs in shared semantic lowering, BIR
  representation, prepared-BIR construction, or a later target backend route.
- Add focused follow-up ideas if the five representative cases split into
  multiple unrelated mechanism gaps.
- Preserve precise diagnostics for constructs that remain outside admitted
  capability.

## Out Of Scope

- Do not hide missing semantic facts inside AArch64 target codegen.
- Do not add target-only workarounds for constructs that fail before backend
  lowering.
- Do not weaken c-testsuite expectations or retag semantic failures as
  unsupported without explicit approval.
- Do not claim AArch64 backend progress from classification-only changes.

## Expected Backend Consumer

After the shared semantic/BIR gap is repaired, the AArch64 backend scan should
receive admitted BIR and prepared facts for the affected cases. Those cases may
then reach AArch64 lowering, assembly/link, runtime-unavailable, or runtime
mismatch buckets.

## Proof Subset To Rerun

Start with:

```text
ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00143_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00185_c|c_testsuite_aarch64_backend_src_00216_c'
```

Then rerun the full AArch64 backend c-testsuite route to confirm the
`[FRONTEND_FAIL]` count and classify any newly exposed backend failures.

## Acceptance Criteria

- Representative `lir_to_bir` failures are grouped by concrete semantic or BIR
  mechanism gap.
- Repairs happen in the shared semantic/BIR/prepared-BIR owner layer before
  target codegen consumes the facts.
- Remaining unsupported constructs retain explicit diagnostics and are not
  hidden behind target-specific fallbacks.

## Reviewer Reject Signals

Reject the route if it patches AArch64 codegen around missing BIR facts,
matches only the representative filenames, weakens c-testsuite expectations,
claims progress from renaming diagnostics or reclassification alone, or leaves
the same `lir_to_bir` failure mode behind a new abstraction name.
