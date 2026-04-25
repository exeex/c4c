# LIR Struct Declaration Printer Authority Switch

Status: Closed
Created: 2026-04-25
Last Updated: 2026-04-25
Closed: 2026-04-25

Parent Ideas:
- [110_lir_struct_type_printer_authority_readiness_audit.md](/workspaces/c4c/ideas/closed/110_lir_struct_type_printer_authority_readiness_audit.md)

## Goal

Switch LLVM struct declaration printing from legacy `LirModule::type_decls` to
structured `LirModule::struct_decls`, while retaining `type_decls` as a
verifier/backcompat proof source.

## Why This Idea Exists

Idea 110 audited the remaining struct/type string surfaces and found one narrow
printer-authority candidate: struct declaration lines rendered from
`struct_decls` through `render_struct_decl_llvm()`. The audit explicitly did
not recommend broader type-ref, global/function/extern signature, call-site, or
backend layout authority changes in the same slice.

## Scope

- Make the LIR LLVM printer emit struct declaration lines from
  `LirModule::struct_decls`.
- Keep `LirModule::type_decls` populated as a legacy proof/backcompat source.
- Keep verifier shadow/parity checks such as `verify_struct_decl_shadows()`
  until a later removal audit proves the legacy line can be deleted.
- Preserve emitted LLVM parity for existing focused struct, template,
  signature, global, extern, ABI, variadic, and aggregate-layout coverage.

## Non-Goals

- Do not remove `type_decls`.
- Do not demote global, function, extern, call, or raw `LirTypeRef` text fields.
- Do not switch backend, BIR, or MIR aggregate layout consumers away from
  `type_decls`.
- Do not broaden HIR-to-LIR layout lookup authority beyond the existing
  structured parity observations.
- Do not rewrite expectations or downgrade tests to make the switch pass.

## Acceptance Criteria

- Struct declarations in emitted LLVM are sourced from `struct_decls`.
- Legacy `type_decls` remains available for verifier/backcompat proof.
- Existing verifier parity continues to reject mismatches between structured
  struct declarations and legacy declaration text.
- Focused coverage from idea 110 still passes, including frontend LIR mirror
  tests and aggregate ABI/layout/runtime tests.
- Any additional test changes prove the semantic authority switch rather than
  matching a single named testcase.

## Closure Notes

Closed after the active runbook switched LLVM struct declaration emission to
`struct_decls`, preserved legacy `type_decls` parity/backcompat proof, proved
verifier mismatch rejection, and passed focused plus full-suite validation.

Close-time regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS, 2980/2980 before and 2980/2980 after.
