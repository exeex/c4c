# HIR Safe Legacy Lookup Demotion

Status: Closed
Created: 2026-04-25
Last Updated: 2026-04-25
Closed: 2026-04-25

Parent Ideas:
- [103_hir_post_dual_path_legacy_readiness_audit.md](/workspaces/c4c/ideas/closed/103_hir_post_dual_path_legacy_readiness_audit.md)

## Goal

Demote or delete only the HIR-internal legacy rendered-name lookup paths that
idea 103 classifies as safe.

This work should keep structured lookup authoritative where metadata is
complete, retain legacy fallback only for classified bridge cases, and avoid
touching downstream HIR-to-LIR or codegen spelling responsibilities.

## Why This Idea Exists

HIR now has structured mirrors for module declarations, compile-time template
registries, enum/const-int values, and struct/method/member owner lookup. The
next cleanup should remove unnecessary HIR-internal reliance on rendered names,
but only after the readiness audit identifies stable candidates.

Deleting all string paths at once would mix semantic cleanup with ABI,
diagnostic, printer, and downstream bridge work. This idea keeps the cleanup
small and behavior-preserving.

## Scope

Use the idea 103 review artifact as the source of truth. Candidate cleanup may
include:

- replacing direct legacy lookup calls with structured-first helpers
- narrowing visibility of `find_function_by_name_legacy` and
  `find_global_by_name_legacy` where all callers have structured IDs or
  `LinkNameId`
- deleting legacy parity-only maps after sufficient proof, if 103 marks them
  safe
- converting HIR-internal method/member/static-member lookup call sites to
  structured owner helpers where the owner key is complete
- preserving fallback counters or diagnostics only where the audit requires
  continued observation

## Out Of Scope

- HIR-to-LIR seam redesign.
- Changing LIR data model strings.
- Removing rendered names needed by ABI, link output, diagnostics, dumps, or
  final textual emission.
- Big-bang deletion of `TypeSpec::tag`.
- Rewriting codegen shared helpers that still require rendered type spellings.
- Parser or sema rewrites.

## Execution Rules

- Do not clean a legacy path unless idea 103 classifies it as `safe-to-demote`
  or `legacy-proof-only`.
- Preserve existing behavior when structured metadata is incomplete.
- Keep `LinkNameId` as the authority for link-visible function/global identity.
- Keep rendered names as spelling output, not semantic lookup authority.
- Add or preserve focused parity proof where a path is demoted but not deleted.

## Acceptance Criteria

- Safe HIR-internal legacy lookup paths identified by idea 103 are demoted or
  removed.
- Bridge-required, diagnostic-only, printer-only, ABI/link spelling, and
  HIR-to-LIR blocked paths remain intact.
- Structured lookup is the primary path for cleaned HIR-internal call sites.
- Focused HIR proof passes without expectation downgrades or testcase-shaped
  shortcuts.
- The final diff does not mix downstream LIR/codegen bridge redesign with
  HIR-internal cleanup.

## Closure Notes

Closed after the Step 6 final-validation packet. The selected safe HIR-internal
demotions are complete for the review-backed scope: complete-`DeclRef` HIR
lookup paths prefer structured lookup, direct method/member probes touched by
the runbook route through parity-recording helpers, and owner-aware HIR record
checks use `find_struct_def_by_owner_structured` when valid owner metadata is
available.

No remaining idea 104 work is blocked inside the HIR-only scope. Retained
legacy paths are deliberately left intact where they are bridge-required,
legacy-proof-only, or still needed for parity observation.

Downstream bridge cleanup remains owned by idea 105, including HIR-to-LIR
string identity, `TypeSpec::tag` codegen/layout consumers, extern filtering,
global best-object selection, rendered `Module::struct_defs`
layout/base/member paths, rendered method/static-member maps, ABI/link spelling,
and fallback-name handoff.

Close validation used the canonical full-suite logs:
`test_before.log` 2976/2976 and `test_after.log` 2976/2976. Regression guard
passed with `--allow-non-decreasing-passed`, appropriate for this lifecycle-only
close after the already-committed validation slice.
