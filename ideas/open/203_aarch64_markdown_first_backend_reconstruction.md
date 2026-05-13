# AArch64 Markdown-First Backend Reconstruction

Status: Open
Created: 2026-05-13

Depends On:
- `ideas/closed/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`
- `ideas/closed/197_bir_backend_compatibility_string_retirement.md`
- `ideas/closed/200_hir_legacy_compatibility_retirement.md`

## Goal

Restart AArch64 backend work by first converting the previous failed
`src/backend/mir/aarch64` implementation surface back into reviewable markdown
drafts, then designing the correct connection to the current BIR /
`PreparedBirModule` contract before growing code back into `.cpp`.

The immediate goal is not to debug or patch the old AArch64 `.cpp` files. The
goal is to make the prior attempt cheap to inspect, identify which pieces are
salvageable, define the real backend interface, and only then reintroduce
implementation files in small coherent slices.

## Why This Idea Exists

The previous AArch64 backend route contains many `.cpp` files under
`src/backend/mir/aarch64`. Editing those files directly would pull the project
back into high-token compile/debug loops before the interface is understood.

The frontend-to-BIR and BIR/backend identity cleanup work has now closed. That
means the next backend route should consume structured BIR/prepared facts
instead of rediscovering semantic identity through strings. A markdown-first
reconstruction lets us compare the old implementation against the current
interface and detect any remaining BIR/prepared gaps before committing to a
new target backend shape.

## In Scope

- Inventory the existing AArch64 files under `src/backend/mir/aarch64`,
  including codegen, assembler, linker, and top-level module entry files.
- Move or copy selected failed implementation `.cpp` content into `.md`
  review artifacts so the old route can be studied without keeping it as live
  production code.
- Build a markdown index that classifies each old AArch64 file as:
  salvageable design note, obsolete route, binary-utils candidate,
  target-ABI candidate, assembler/linker candidate, or delete/defer.
- Define the intended new interface from current backend data:
  - preferred entry should be `PreparedBirModule` unless a narrower raw BIR
    slice is explicitly justified
  - semantic identity must use structured ids already present in BIR/prealloc
  - no new rendered-name recovery fallback may be introduced
  - target-local MIR/asm structures should be specified before codegen grows
- Produce a BIR/prepared interface gap ledger. If a required fact is missing
  from BIR or `PreparedBirModule`, stop backend implementation and create a
  separate BIR/prepared gap idea before continuing.
- After the markdown review converges, choose the first tiny implementation
  slice to grow back into `.cpp`.

## Out Of Scope

- Directly fixing the old AArch64 `.cpp` implementation before the markdown
  review and interface ledger are complete.
- Starting broad AArch64 instruction selection, register allocation, assembler,
  or linker rewrites in this idea.
- Adding string-based semantic recovery to make old backend code fit the new
  BIR/prepared interface.
- Weakening backend tests or marking supported cases unsupported to claim
  progress.
- Reopening parser, sema, HIR, LIR, or BIR identity cleanup unless a concrete
  missing carrier is discovered and split into its own source idea.

## Acceptance Criteria

- The old AArch64 implementation surface has a markdown review index that
  explains which files are retained as design notes, deferred, or candidates
  for future code regeneration.
- Live `.cpp` backend work is not expanded before the interface study is
  complete.
- The proposed backend entry contract states whether the new route consumes
  `PreparedBirModule`, raw `bir::Module`, or a staged subset, and why.
- The BIR/prepared gap ledger is complete enough to decide whether backend
  work may proceed or must first open a BIR/prepared carrier idea.
- Any new retained compatibility or deprecated route discovered during the
  markdown study is documented with a comment or ledger entry containing
  `legacy` or `deprecated`, plus owner, limitation, and removal condition.
- The next implementation idea is small, code-oriented, and starts only after
  the markdown-first contract is accepted.

## Reviewer Reject Signals

- The route starts debugging or expanding AArch64 `.cpp` files before the
  markdown interface ledger is complete.
- The markdown conversion becomes a blind file rename without design
  classification or backend contract analysis.
- The proposed backend route adds rendered-string lookup or name recovery
  fallbacks instead of consuming structured BIR/prepared identity.
- A discovered missing BIR/prepared fact is worked around inside AArch64
  target code instead of being split into a BIR/prepared gap idea.
- The first implementation slice is broad enough to require large compile/debug
  loops before the target interface is proven.
- Tests are weakened, skipped, or reclassified to make the old failed backend
  route appear viable.
