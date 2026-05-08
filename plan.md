# Parser Sema Record Tag Compatibility Table Retirement Runbook

Status: Active
Source Idea: ideas/open/153_parser_sema_record_tag_compatibility_table_retirement.md

## Purpose

Retire parser rendered-record-tag compatibility tables as semantic authority by making parser record carriers consistently feed Sema-owned record-domain tables.

Goal: make parser record identity and completion prefer structured record carriers, owner metadata, and Sema record-domain lookup before rendered `std::string` tags or mangled names.

## Core Rule

Rendered record spelling is not semantic record identity. It may remain only for diagnostics, AST legacy mirrors, template instantiation display names, compatibility probes, or documented temporary fallbacks with an owner, limitation, and removal condition.

## Read First

- `ideas/open/153_parser_sema_record_tag_compatibility_table_retirement.md`
- `src/frontend/parser/`
- `src/frontend/sema/`
- Parser support code that references `TypeSpec::record_def`, `tag_text_id`, qualifier `TextId`s, namespace/global context, or owner keys.
- Focused frontend/parser/sema tests that exercise same-spelling records, nested records, template instantiated records, member typedefs, or `__builtin_types_compatible_p` behavior.

## Current Targets

- `ParserDefinitionState::defined_struct_tags`
- `ParserDefinitionState::struct_tag_def_map`
- Parser reads and writes that treat rendered record tags as completion or identity authority.
- Template instantiated record insertion and lookup paths that key records by mangled rendered names.
- Member typedef and record carrier paths that can preserve `record_def`, `tag_text_id`, qualifier metadata, namespace/global context, or owner keys instead of reparsing rendered spelling.

## Non-Goals

- Do not remove diagnostic, dump, `Node::name`, `Node::unqualified_name`, or final display spelling when structured metadata exists beside it.
- Do not treat `TextId` alone as final record identity across scopes.
- Do not reopen HIR/LIR aggregate layout cleanup already covered by idea 152.
- Do not redesign unrelated Sema record or type systems.
- Do not weaken tests, mark supported record cases unsupported, or rewrite expectations only to match changed rendered names.
- Do not hide rendered-tag semantic authority behind a renamed helper and claim retirement.

## Working Model

- Parser carries partial syntax structure: record kind, source spelling `TextId`, namespace/global qualifier context, qualifier `TextId`s, and provisional `record_def` when locally known.
- Sema owns final record-domain meaning: record identity, completion, tag/typedef/namespace disambiguation, owner-indexed record tables, and canonical lookup from parser-produced carriers.
- Rendered record tags may mirror spelling for diagnostics, compatibility, and legacy AST surfaces, but they must not override complete structured metadata.
- Template instantiation should preserve structured owner/key metadata for instantiated records and member typedefs before using mangled display names.

## Execution Rules

- Start with an inventory of every parser read/write of `struct_tag_def_map` and `defined_struct_tags`.
- Classify each use as semantic authority, compatibility mirror, final/display spelling, test hook, or temporary fallback for incomplete metadata.
- Replace decision authority before deleting compatibility mirrors.
- Prefer semantic lowering through structured carriers and Sema record-domain lookup over testcase-shaped matching.
- Every code-changing step needs fresh build or narrow compile/test proof chosen by the supervisor.
- Escalate validation when record identity, template instantiated record lookup, member typedefs, or parser/sema record-domain behavior changes.

## Step 1: Inventory Parser Record Tag Authority

Goal: identify every rendered-record-tag table use and choose the first narrow implementation packet.

Primary targets:
- `src/frontend/parser/`
- `src/frontend/sema/`
- parser support helpers that consume record tag maps

Actions:
- Search for `defined_struct_tags`, `struct_tag_def_map`, rendered tag lookup, mangled record tag keys, and record compatibility probes.
- Classify each finding as semantic authority, compatibility mirror, final/display spelling, test hook, temporary fallback, or unrelated.
- Map each in-scope semantic route to one owner: parser record carrier production, Sema record-domain lookup, template instantiated record registration, member typedef path, or parser support compatibility.
- Record the first implementation packet, owned files, and supervisor-delegated proof command in `todo.md` before code changes begin.

Completion check:
- `todo.md` names the concrete first implementation packet, owned files, and proof command.
- The inventory distinguishes in-scope semantic fallback from allowed display or diagnostic rendering.
- No source idea edit is needed unless the inventory contradicts durable source intent.

## Step 2: Demote Parser Record Tag Maps To Mirrors

Goal: make ordinary parser record identity checks prefer structured record carriers before `struct_tag_def_map` or `defined_struct_tags`.

Actions:
- Inspect parser record definition and lookup paths that already have `TypeSpec::record_def`, `tag_text_id`, qualifier metadata, namespace/global context, or owner keys.
- Route covered record identity checks through those structured carriers or Sema record-domain lookup before rendered tag maps.
- Keep retained rendered tag maps explicitly documented as compatibility/display/test mirrors.
- Add or update focused coverage for stale or ambiguous rendered record tags where structured identity must win.

Completion check:
- Covered parser record semantic lookup no longer uses rendered tag maps as first authority when structured metadata is available.
- Retained map uses are classified with owner, limitation, and removal condition.

## Step 3: Retire Template Instantiated Record Rendered Keys

Goal: ensure template instantiated record identity does not depend on mangled rendered record tag strings when structured owner/key metadata is available.

Actions:
- Inspect template instantiated record insertion, cloning, and lookup paths that generate or consume mangled rendered record names.
- Preserve or construct structured owner keys, record carriers, `tag_text_id`, qualifier metadata, and template instantiation context for the targeted paths.
- Replace semantic lookup by mangled rendered key with structured record-domain lookup.
- Keep mangled names only as display/final spelling or a documented compatibility mirror.
- Add coverage for same-spelling or stale rendered template record tags that would bind incorrectly through string authority.

Completion check:
- Targeted template instantiated records resolve through structured metadata before rendered mangled names.
- Remaining mangled-name uses are display or compatibility only.

## Step 4: Repair Member Typedef And Record Carrier Handoffs

Goal: make member typedef and nearby record carrier handoffs preserve structured record identity across parser/sema boundaries.

Actions:
- Inspect member typedef paths that insert, clone, or probe record tags through legacy rendered strings.
- Ensure handoffs carry provisional `record_def`, source `tag_text_id`, qualifier `TextId`s, namespace/global context, and owner keys when available.
- Replace rendered tag probes with Sema-owned record-domain lookup for covered paths.
- Add focused coverage where stale rendered member record spelling would otherwise choose the wrong record.

Completion check:
- Targeted member typedef and record handoff paths preserve structured identity through parser/sema lookup.
- Any remaining rendered fallback is explicit, secondary, and documented.

## Step 5: Audit Parser Support Record Compatibility Bridges

Goal: keep parser support helpers from restoring rendered tag maps as semantic record authority after structured metadata misses.

Actions:
- Inspect parser support helpers that receive record or typedef maps and any parser call sites that pass `struct_tag_def_map` or rendered tags.
- Separate legitimate compatibility probes from semantic interpretation that belongs in Sema record-domain tables.
- Add structured or domain-key handoff where parser-owned callers already have complete metadata.
- Document retained compatibility wrappers with owner, limitation, and removal condition.

Completion check:
- Parser support does not override complete structured record metadata through rendered map fallback.
- Compatibility wrappers are named and documented as compatibility, not ordinary semantic environments.

## Step 6: Validate And Review Remaining Record Compatibility

Goal: prove the source idea acceptance criteria or leave a precise lifecycle note for any remaining scoped compatibility route.

Actions:
- Re-inventory `struct_tag_def_map`, `defined_struct_tags`, rendered record tag probes, and mangled rendered record key use.
- Check reviewer reject signals from the source idea, especially renamed string authority, raw `TextId`-alone identity, template records keyed only by mangled spelling, and weakened tests.
- Run the supervisor-selected focused proof and any broader regression needed for changed parser/sema/template behavior.
- Record remaining compatibility owners, limitations, and removal conditions in `todo.md` for lifecycle review.

Completion check:
- Acceptance criteria from idea 153 are satisfied or the remaining in-scope routes are explicitly identified for the next lifecycle decision.
- Regression proof is fresh.
- No testcase-overfit or unsupported expectation downgrade is accepted as progress.
