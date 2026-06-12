# 236 Phase E3 prepared compare-join selected-value-chain metadata follow-up

## Goal

Repair prepared compare-join selected-value-chain metadata so pointer-backed
same-module global return-context ownership keeps publishing the true
global-root selected-value chain.

## Why This Exists

After the dirty idea 234 Step 2 work advanced past earlier assertions,
`backend_x86_handoff_boundary` failed later at:

```text
scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global selected-value chain return context ownership:
shared helper stopped publishing the true global-root selected-value chain
```

The reviewer judged this as a separate prepared compare-join metadata
initiative, likely around `src/backend/prealloc/control_flow.hpp`, not as part
of idea 234's stack-backed parameter-home handoff repair.

## In Scope

- Prepared compare-join selected-value-chain metadata for pointer-backed
  same-module global return contexts.
- The shared helper path that publishes or preserves the true global-root
  selected-value chain through compare-join return context ownership.
- Focused helper and handoff-boundary proof for the pointer-backed
  selected-value-chain assertion and nearby same-feature metadata cases.
- Preservation of existing x86 stack-home handoff and Route 6 route-debug
  behavior as regression guards.

## Out Of Scope

- x86 compare-join stack-home handoff repair from idea 234.
- Route 6 consumed scalar i32 call-argument source repair from idea 235.
- Broad pointer lowering rewrites, ABI/call wrapper migration, prepared
  diagnostic/oracle retirement, or baseline refreshes.
- Testcase-name or assertion-string shortcuts for the pointer-backed global
  selected-value-chain fixture.

## Acceptance Criteria

- The pointer-backed same-module global selected-value-chain ownership case
  publishes the true global-root selected-value chain through semantic prepared
  compare-join metadata.
- Nearby same-feature selected-value-chain metadata cases continue to preserve
  the intended root and return-context ownership.
- `backend_x86_handoff_boundary` advances past the selected-value-chain
  assertion without weakening it, and the supervisor-selected Route 6 and
  stack-home guard subsets remain green.
- Any shared prepared control-flow, helper-oracle, MIR query, or direct-call
  surface touched by the repair receives supervisor-selected broader
  validation.

## Reviewer Reject Signals

- The change matches the pointer-backed global testcase name, label text,
  block shape, or assertion string instead of repairing selected-value-chain
  metadata publication.
- The slice weakens, removes, or marks unsupported the selected-value-chain
  ownership assertion.
- The slice claims progress through expected-string rewrites, baseline
  refreshes, helper renames, or classification-only changes.
- The implementation changes Route 6 consumed-plan behavior or x86
  stack-home compare-join consumer behavior as part of this metadata repair.
- The exact old global-root selected-value-chain failure remains present
  behind a renamed helper or alternate diagnostic path.

