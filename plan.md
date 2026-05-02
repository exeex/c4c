# HIR Legacy String Lookup Metadata Resweep Runbook

Status: Active
Source Idea: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md

## Purpose

Run a focused HIR cleanup pass for rendered-string lookup authority and
metadata propagation after parser/Sema rendered lookup removal exposed
cross-module carrier gaps.

## Goal

Make covered HIR semantic lookup paths structured-primary while preserving
rendered names only for diagnostics, dumps, final spelling, mangling payloads,
link-visible spelling, or explicit no-metadata compatibility fallback.

## Core Rule

Do not replace rendered-string semantic lookup with renamed rendered-string
lookup. Where parser/Sema or HIR already has declaration ids, `TextId`,
namespace qualifiers, `ModuleDeclLookupKey`, `HirRecordOwnerKey`, `LinkNameId`,
or structured record/template/NTTP carriers, use those carriers as authority.

## Read First

- `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md`
- HIR lookup and fallback surfaces in `src/frontend/hir`
- Parser/Sema consteval and record-layout handoff callers only where needed to
  understand HIR producer/consumer contracts

## Current Targets

- HIR string-keyed lookup maps and rendered-name fallback paths.
- `find_function_by_name_legacy`, `find_global_by_name_legacy`,
  `has_legacy_mangled_entry`, `ModuleDeclLookupAuthority::LegacyRendered`,
  `declaration_fallback`, rendered-name compatibility indexes, and
  `fallback_tag` / `fallback_member` style recovery.
- HIR `NttpBindings` and equivalent consteval metadata handoff.
- Structured HIR record-layout carrier for Sema consteval layout lookup.
- Tests proving structured metadata wins when rendered spelling drifts.

## Non-Goals

- Parser/Sema cleanup except to record or bridge a HIR-owned blocker.
- LIR, BIR, backend, or codegen cleanup except to create follow-up open ideas
  when a downstream carrier gap is exposed.
- Removing diagnostics, dumps, mangling, ABI/link-visible text, or final
  spelling merely because they are strings.
- Weakening tests, marking supported routes unsupported, or adding named-case
  shortcuts.

## Working Model

- Classify every retained HIR string surface before changing behavior.
- Prefer deleting or bypassing legacy rendered lookup when a structured key is
  already present.
- Keep explicit no-metadata compatibility routes narrow and named as
  compatibility, not as normal metadata-backed lookup.
- Split missing upstream parser/Sema metadata or downstream lowering needs into
  separate `ideas/open/` files instead of widening this plan.

## Execution Rules

- Make small, buildable steps and update `todo.md` after executor packets.
- Keep source idea edits rare; routine findings belong in `todo.md`.
- Add tests for same-feature structured-vs-rendered disagreement where a route
  is migrated.
- Treat expectation downgrades, unsupported rewrites, and named-fixture-only
  proof as route failures.
- For code-changing steps, at minimum run `cmake --build build --target c4cll`
  and the narrow relevant frontend/HIR tests chosen by the supervisor.

## Step 1: Inventory HIR Rendered Lookup Surfaces

Goal: build the working map of HIR string-keyed semantic lookup and fallback
surfaces before migrating behavior.

Primary Target: `src/frontend/hir`

Actions:
- Search for `std::string` keyed maps, rendered-name fallback paths, legacy
  lookup APIs, compatibility indexes, `fallback_tag`, `fallback_member`, and
  semantic uses of rendered names.
- Classify each retained string surface as semantic lookup, diagnostics, dump,
  final spelling, mangling/link spelling, compatibility, local scratch, or
  unresolved metadata boundary.
- Identify which routes already have structured alternatives such as
  declaration ids, `TextId`, `ModuleDeclLookupKey`, `HirRecordOwnerKey`, or
  `LinkNameId`.

Completion Check:
- `todo.md` records the classified HIR surface map and names the first concrete
  migration target.
- No behavior change is required for this step unless a trivial classification
  cleanup is needed to make the next step unambiguous.

## Step 2: Migrate HIR NTTP And Consteval Handoff

Goal: remove metadata-backed dependence on string-keyed `NttpBindings` and
equivalent consteval handoff paths.

Primary Target: HIR compile-time and consteval code that passes NTTP or
constant metadata through rendered string maps.

Actions:
- Trace parser/Sema metadata into HIR lowering for NTTP values and consteval
  calls.
- Introduce or reuse structured or `TextId` keyed carriers where producer
  metadata already exists.
- Keep any no-metadata compatibility path explicit and separate from the
  metadata-backed route.
- Do not delete parser/Sema compatibility overloads until the HIR caller
  contract no longer requires them for metadata-backed routes.

Completion Check:
- Metadata-backed NTTP/consteval routes no longer require rendered
  `std::unordered_map<std::string, long long>` lookup authority.
- Build proof and focused tests cover at least one structured-vs-rendered
  disagreement route.

## Step 3: Add Structured HIR Record-Layout Handoff

Goal: let Sema consteval record-layout lookup use structured HIR record-layout
authority when record owner metadata is available.

Primary Target: HIR record definition ownership/indexing and the Sema consteval
record-layout handoff.

Actions:
- Trace how HIR exposes record definitions and layout data through
  `struct_def_owner_index` and `find_struct_def_by_owner_structured(...)`.
- Add or migrate a structured record-layout carrier so metadata-backed lookup
  does not depend on rendered `TypeSpec::tag` and rendered
  `ConstEvalEnv::struct_defs` keys.
- Classify any retained rendered-keyed layout path as no-metadata
  compatibility.

Completion Check:
- Metadata-backed consteval record-layout lookup can use structured HIR record
  owner/index data.
- Tests cover structured layout lookup winning over stale or drifted rendered
  spelling.

## Step 4: Convert Or Demote Legacy HIR Lookup Routes

Goal: remove normal-path semantic authority from legacy rendered HIR lookup
routes where structured keys already exist.

Primary Target: HIR declaration/function/global/template/member lookup helpers
and compatibility indexes.

Actions:
- Convert callers of `find_function_by_name_legacy`,
  `find_global_by_name_legacy`, `ModuleDeclLookupAuthority::LegacyRendered`,
  `has_legacy_mangled_entry`, `declaration_fallback`, and rendered-name
  compatibility indexes to structured authority where available.
- Delete obsolete rendered lookup paths when no compatibility caller remains.
- Keep required no-metadata compatibility routes narrow, named, and tested.

Completion Check:
- Covered metadata-backed HIR declaration lookup paths are structured-primary.
- Any retained legacy rendered route is explicitly classified and outside the
  normal metadata-backed path.

## Step 5: Preserve Display And Link Text Boundaries

Goal: make sure semantic cleanup does not remove legitimate rendered text
payloads.

Primary Target: diagnostics, dumps, mangling, final spelling, ABI/link-visible
names, and explicit compatibility payloads touched by earlier steps.

Actions:
- Audit changed routes for accidental loss of diagnostic, dump, final spelling,
  mangling, or link-visible output.
- Keep link and display strings as payloads, not lookup authority.
- Add focused assertions where a display string remains visible while
  structured metadata decides identity.

Completion Check:
- Remaining string surfaces are classified as display, diagnostics, dump, final
  spelling, compatibility, local scratch, or unresolved metadata boundary.
- No tests are weakened to hide display/link regressions.

## Step 6: Record Boundaries And Validate

Goal: close the HIR resweep runbook only after blockers and follow-up
boundaries are explicit.

Primary Target: validation logs, `todo.md`, and any required new
`ideas/open/*.md` follow-up files.

Actions:
- Create separate open ideas for missing upstream parser/Sema metadata or
  downstream LIR/BIR/backend carrier needs exposed during execution.
- Run the supervisor-selected broader validation once the covered HIR routes
  are migrated.
- Confirm no retained route violates the source idea reviewer reject signals.

Completion Check:
- `cmake --build build --target c4cll` passes.
- Focused frontend/HIR tests cover migrated structured-vs-rendered
  disagreement routes.
- Follow-up metadata blockers are represented as open ideas instead of hidden
  inside this runbook.
