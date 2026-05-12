# BIR Backend Compatibility String Retirement Runbook

Status: Active
Source Idea: ideas/open/197_bir_backend_compatibility_string_retirement.md

## Purpose

Retire or hard-fence remaining BIR and backend-prepared compatibility string
authority so future target backend work consumes structured identity instead
of rediscovering semantics from rendered spelling.

## Goal

Metadata-rich BIR, prealloc, and backend-prepared paths must use structured ids
for text, link-visible symbols, and layout identity, or fail closed behind an
explicit no-id compatibility boundary.

## Core Rule

Do not claim progress by renaming raw string lookups. Covered metadata-rich
paths must stop recovering through identical rendered spelling after a
structured id mismatch or miss.

## Read First

- `ideas/open/197_bir_backend_compatibility_string_retirement.md`
- Existing BIR string, type, layout, prealloc, and backend-prepared lookup
  comments that mention compatibility, legacy, no-id, rendered, display, or
  diagnostic spelling.
- Existing focused tests around BIR string constants, aggregate layout lookup,
  globals, functions, and backend preparation.

## Current Targets And Scope

- `bir::StringConstant::name` and related text-pool/string-constant identity.
- Structured type spelling and aggregate layout lookup bridges.
- `GlobalTypes`, `TypeDeclMap`, and LIR-boundary compatibility maps.
- Backend-prepared route-local name lookups that could be mistaken for
  semantic identity.
- Link-visible global, function, and pointer-to-symbol identity that should
  consume `LinkNameId` when metadata is available.
- Route-local SSA, slot, block, value, and debug-focus spellings only as
  explicitly route-local state.

## Non-Goals

- Do not start or complete the AArch64 backend implementation.
- Do not remove strings required by printers, diagnostics, assemblers,
  linkers, debug focus, or final object/output spelling.
- Do not rewrite the complete aggregate layout system in one pass.
- Do not change parser, Sema, HIR, or LIR source intent except to record a
  separate follow-up idea if a missing upstream carrier blocks BIR cleanup.
- Do not weaken supported behavior or rewrite expectations to hide a
  compatibility gap.

## Working Model

- Treat `TextId` as authority for text-pool or string-constant identity.
- Treat `LinkNameId` as authority for link-visible global, function, or
  pointer-to-symbol identity.
- Treat final spelling, diagnostics, dump text, assembler output, linker
  spelling, route-local SSA names, slots, blocks, and debug focus labels as
  observation text unless an existing structured carrier says otherwise.
- Retained raw/no-id paths must be named as compatibility and must state their
  owner, limitation, and removal condition.

## Execution Rules

- Prefer small semantic slices: inventory, fence, prove, then delete or convert.
- Add or update tests before accepting a compatibility change whenever a stale
  rendered spelling would previously have succeeded.
- A structured id mismatch on a covered metadata-rich path must fail closed; it
  must not fall back to matching raw strings.
- Keep route-local generated names route-local. Do not promote them to semantic
  authority.
- If a missing upstream carrier is required, record it as a separate
  `ideas/open/` initiative instead of expanding this plan into frontend work.
- For code-changing steps, prove with `build` plus the supervisor-selected
  narrow CTest subset; escalate to broader validation after shared lookup or
  backend-prepared interfaces change.

## Step 1: Inventory BIR And Backend Compatibility Strings

Goal: build a current, grep-friendly map of remaining raw string authority in
BIR and backend-prepared interfaces.

Primary targets:
- BIR string constant and text-pool structures.
- Type/layout lookup maps and LIR-boundary handoff maps.
- Backend preparation and prealloc helpers that accept or derive raw names.

Actions:
- Inspect retained fields and helpers that use raw string keys next to
  structured ids.
- Classify each retained string as one of: semantic compatibility,
  final/output spelling, diagnostic/display, route-local handle, or explicit
  no-id compatibility.
- Identify one narrow metadata-rich path where stale or missing structured id
  behavior can be tested before conversion.

Completion check:
- `todo.md` records the inventory summary and the first conversion/fencing
  target, with no source idea rewrite.

## Step 2: Fence Or Convert Text-Pool And String-Constant Identity

Goal: prevent BIR string constants and text-pool identities from relying on raw
string lookup when metadata is present.

Primary targets:
- `bir::StringConstant::name`
- Text-pool/string-constant lookup helpers and generated metadata handoff.

Actions:
- Convert metadata-rich string-constant identity to `TextId` where the value
  denotes text-pool/string-constant identity.
- Fence any remaining raw path as explicit no-id compatibility with owner and
  removal condition.
- Add focused stale-id or missing-id proof for the converted or fenced path.

Completion check:
- Metadata-rich text/string-constant lookup no longer recovers through raw
  spelling on a covered structured miss, and the narrow proof is recorded.

## Step 3: Convert Link-Visible Symbol Identity

Goal: make BIR, prealloc, and backend-prepared global/function identity consume
`LinkNameId` authority when metadata is available.

Primary targets:
- Global/function lookup maps.
- Pointer-to-symbol and backend-prepared symbol handoff.
- `GlobalTypes` or related maps when used as semantic link-visible identity.

Actions:
- Replace raw symbol-name authority with `LinkNameId` on metadata-rich paths.
- Preserve final output/linker spelling only at final spelling boundaries.
- Add fail-closed proof for stale or mismatched link-name metadata.

Completion check:
- Covered symbol lookups reject stale structured metadata instead of recovering
  through identical rendered spelling.

## Step 4: Fence Type And Aggregate Layout Compatibility

Goal: stop structured type/layout lookup from treating rendered type spelling
as ordinary semantic authority.

Primary targets:
- Structured type spelling bridges.
- Aggregate layout lookup maps.
- `TypeDeclMap` and LIR-boundary compatibility maps.

Actions:
- Convert layout/type compatibility maps to structured ids where local carriers
  already exist.
- Fence unavoidable rendered-spelling paths as explicit compatibility, with
  owner, limitation, and removal condition.
- Add focused proof for stale type/layout spelling that previously could have
  recovered.

Completion check:
- Covered type/layout metadata-rich misses fail closed or enter only named
  no-id compatibility, with proof recorded.

## Step 5: Audit Backend-Prepared Route-Local Names

Goal: keep SSA, slot, block, value, and debug-focus names from crossing into
semantic identity.

Primary targets:
- Backend-prepared route-local lookup tables.
- Prealloc local handles and debug focus naming.
- Any route-local maps used near semantic lookup code.

Actions:
- Confirm route-local names do not escape as semantic keys across BIR,
  prepared-module, MIR, or LIR boundaries.
- Rename or comment retained route-local spellings only where that clarifies
  display/debug/local ownership.
- Avoid broad backend rewrite or new target backend implementation work.

Completion check:
- Retained route-local names are documented or structured so backend restart
  work cannot treat them as semantic authority.

## Step 6: Closure Ledger And Broader Proof

Goal: leave a reviewer-auditable ledger for backend restart consumers.

Actions:
- Summarize deleted, converted, fenced, and intentionally retained string paths
  in `todo.md`.
- State whether new backend restart work may consume the BIR/prepared interface
  without adding rendered-name recovery fallbacks.
- Run the supervisor-selected broader validation for the final compatibility
  retirement slice.

Completion check:
- The ledger covers final spelling, diagnostics, route-local handles, and
  explicit raw/no-id compatibility.
- Focused proofs cover structured success, stale-id fail-closed behavior, and
  any retained no-id compatibility path.
