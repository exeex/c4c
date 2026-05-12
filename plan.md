# Parser Legacy Compatibility Retirement Runbook

Status: Active
Source Idea: ideas/open/198_parser_legacy_compatibility_retirement.md

## Purpose

Retire or hard-fence parser-owned legacy compatibility paths so parser output
carries structured syntax and identity facts without treating rendered strings
as a second semantic route.

## Goal

Parser metadata-rich routes must use structured carriers such as `TextId`,
qualifier segments, direct `record_def` links, Sema-owned record-domain lookup,
or explicit no-metadata compatibility boundaries.

## Core Rule

Do not claim progress by renaming parser compatibility helpers. Covered
metadata-rich parser paths must stop recovering record, owner, qualified-name,
or const-int identity through rendered spelling after a complete structured
miss.

## Read First

- `ideas/open/198_parser_legacy_compatibility_retirement.md`
- Parser source comments mentioning compatibility, legacy, deprecated,
  rendered, display, diagnostic, no-metadata, tag maps, qualified names, or
  named constants.
- Existing parser, Sema-facing parser, template instantiation, and consteval
  tests that exercise record tags, qualified names, source spelling, and
  named constants.

## Current Targets And Scope

- `resolve_record_type_spec_with_parser_tag_map_compatibility`
- `parser_record_layout_compatibility_tag_map`
- `eval_const_int_with_rendered_named_const_compatibility`
- Retained `struct_tag_def_map` and `defined_struct_tags` parser probes.
- Parser-local record setup that lacks structured qualified-name metadata and
  falls back to rendered tag sets.
- Template instantiated record lookup that derives candidate keys from
  `template_origin_name`.
- Rendered owner or tag recovery in parser expression construction when
  structured owner metadata is incomplete.

## Non-Goals

- Do not rewrite Sema-owned record-domain lookup.
- Do not rework HIR, LIR, BIR, or backend consumers.
- Do not remove parser diagnostics, AST display strings, source spelling
  payloads, or final output text.
- Do not treat a raw `TextId` alone as complete semantic identity across
  scopes.
- Do not weaken tests or change expectations to hide unresolved parser
  compatibility paths.

## Working Model

- Structured parser carriers include qualifier segment metadata, direct
  `record_def` links, source `TextId` payloads when domain-scoped, and
  Sema-owned record-domain lookup results.
- Rendered tag names, rendered qualified spelling, parser tag maps, and
  rendered named-const lookup are compatibility authority only when complete
  structured metadata is unavailable.
- Retained parser-local scratch state must be named as compatibility,
  display/diagnostic/source spelling, or no-metadata behavior, with owner,
  limitation, and removal condition.
- A complete structured miss in a covered parser route must fail closed or
  enter an explicit no-metadata boundary; it must not silently recover through
  rendered spelling.

## Execution Rules

- Prefer small semantic steps: inventory, fence one route, prove stale-rendered
  behavior, then delete or convert the next route.
- Add or update focused tests before accepting a parser compatibility change
  whenever stale rendered spelling would previously have succeeded.
- Keep routine findings in `todo.md`; edit the source idea only if durable
  source intent changes or a separate initiative must be recorded.
- Newly discovered retained parser bridges need nearby source comments
  containing `legacy` or `deprecated`, plus owner, limitation, and removal
  condition.
- If Sema, HIR, BIR, or backend work is required, create or request a separate
  open idea instead of expanding this parser-owned plan.
- For code-changing steps, prove with a fresh build plus the
  supervisor-selected narrow parser/frontend CTest subset; escalate to broader
  validation after shared parser identity interfaces change.

## Step 1: Inventory Parser Compatibility Routes

Goal: build a current, grep-friendly map of parser-owned rendered-string
compatibility authority.

Primary targets:
- Record type resolution compatibility helpers and parser tag maps.
- Parser-local record setup and qualified-name metadata handoff.
- Named const and const-int evaluation paths that still accept rendered names.
- Template instantiated record lookup and expression construction owner/tag
  recovery.

Actions:
- Inspect parser-owned helpers and source comments that mention compatibility,
  legacy, deprecated, rendered, tag maps, qualified names, or named constants.
- Classify each retained path as metadata-rich semantic lookup, no-metadata
  compatibility, parser-local scratch state, source/display/diagnostic
  spelling, or separate follow-up work.
- Identify the first narrow stale-rendered parser path that can prove
  fail-closed behavior before or alongside conversion.

Completion check:
- `todo.md` records the parser compatibility inventory and first conversion or
  fencing target, with no source idea rewrite.

## Step 2: Fence Record Type And Layout Tag Map Compatibility

Goal: stop metadata-rich parser record and layout routes from recovering
through parser tag maps after structured metadata misses.

Primary targets:
- `resolve_record_type_spec_with_parser_tag_map_compatibility`
- `parser_record_layout_compatibility_tag_map`
- Retained `struct_tag_def_map` and `defined_struct_tags` probes.

Actions:
- Convert callers with complete structured record metadata to direct
  structured lookup or fail-closed behavior.
- Fence unavoidable parser-local tag-map probes as explicit no-metadata
  compatibility with owner, limitation, and removal condition.
- Add focused stale-rendered record or layout proof for the converted or
  fenced route.

Completion check:
- Covered metadata-rich record/layout misses no longer recover through
  rendered parser tag maps, and narrow proof is recorded in `todo.md`.

## Step 3: Convert Parser Qualified-Name And Owner Recovery

Goal: ensure parser record setup and expression construction use structured
qualified-name and owner metadata when it is complete.

Primary targets:
- Parser-local record setup that lacks structured qualified-name metadata.
- Rendered owner/tag recovery in parser expression construction.
- Source-spelling fields that sit near semantic owner lookup.

Actions:
- Thread or consume existing structured qualifier, owner, or `record_def`
  carriers where parser routes already have them.
- Keep source spelling and diagnostic/display names as observation text.
- Fence incomplete-metadata fallback as parser-local compatibility only.
- Add proof that stale rendered owner or qualified spelling cannot override
  complete structured metadata.

Completion check:
- Covered parser owner/qualified-name routes either use structured carriers or
  fail closed behind explicit compatibility.

## Step 4: Retire Rendered Named-Const Compatibility

Goal: prevent parser const-int evaluation from accepting rendered named-const
identity after complete structured metadata misses.

Primary targets:
- `eval_const_int_with_rendered_named_const_compatibility`
- Parser-facing named-constant lookup or consteval handoff surfaces.

Actions:
- Prefer structured named-constant metadata or domain-scoped `TextId` carriers
  when present.
- Fence retained rendered named-const lookup as no-metadata compatibility, or
  delete it when no production caller still needs it.
- Add focused stale-rendered named-const proof.

Completion check:
- Covered parser const-int routes do not recover through rendered named consts
  after complete structured misses.

## Step 5: Audit Template Instantiated Record Lookup

Goal: prevent template instantiated record lookup from deriving semantic
authority from rendered `template_origin_name` when structured metadata is
available.

Primary targets:
- Template instantiated record lookup candidate-key construction.
- Parser/Sema handoff surfaces for instantiated records.

Actions:
- Replace rendered-origin candidate keys with structured record, owner, or
  domain metadata where available.
- Fence remaining rendered-origin lookup as legacy/no-metadata compatibility
  with a concrete removal condition.
- Add proof for stale rendered template-origin names if the route remains
  reachable.

Completion check:
- Metadata-rich instantiated record lookup does not recover through stale
  rendered origin names.

## Step 6: Closure Ledger And Broader Parser Proof

Goal: leave a reviewer-auditable ledger for parser compatibility retirement.

Actions:
- Summarize deleted, converted, fenced, and intentionally retained parser
  compatibility routes in `todo.md`.
- State any follow-up work that belongs outside the parser domain as a
  separate open idea rather than hidden plan expansion.
- Run the supervisor-selected broader parser/frontend validation for the final
  compatibility retirement slice.

Completion check:
- The ledger covers parser record, layout, qualified-name, owner, const-int,
  and template instantiated record routes.
- Focused proofs cover structured success, stale-rendered fail-closed
  behavior, and any retained no-metadata compatibility path.
