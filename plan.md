# Parser/Sema Post-Cleanup Structured Identity Leftovers Runbook

Status: Active
Source Idea: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md

## Purpose

Clean up parser and sema structured-identity leftovers found by idea 97 without
expanding into HIR data-model migration.

## Goal

Make parser/sema-owned leftover lookup paths prefer structured or `TextId`
identity where stable metadata is available, while preserving rendered-string
bridges required by AST, HIR, consteval, diagnostics, codegen, and link-name
output.

## Core Rule

Keep this runbook limited to parser/sema-owned cleanup. Do not start HIR module
lookup migration, reinterpret `TypeSpec::tag`, remove bridge-required rendered
maps, downgrade expectations, or add testcase-shaped exceptions.

## Read First

- `AGENTS.md`
- `todo.md`
- `ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md`
- `review/97_structured_identity_completion_audit.md`
- Parser helper surfaces in `src/frontend/parser/` and `src/frontend/impl/`
- Sema validation and consteval surfaces in `src/frontend/sema/`

## Current Targets

- Parser const-int evaluation helper overloads and parser-owned callers where
  a stable `TextId` is available.
- Parser typedef-chain and type-compatibility helper overloads only where the
  caller already has structured identity.
- Parser/AST enum definition metadata needed to carry per-enumerator `TextId`
  identity from enum parsing into sema.
- Sema enum variant text/key mirrors for global and local bindings.
- Parser/AST template parameter metadata needed to carry per-parameter
  `TextId` identity from template parsing into sema.
- Sema template NTTP placeholder and validation binding mirrors.
- Sema template type-parameter validation mirrors.
- Consteval call NTTP binding text/key maps.
- Type-binding text mirror plumbing that is currently declared/threaded but
  unused.

## Non-Goals

- No HIR module maps, compile-time engine registries, or HIR lowerer symbol map
  migration.
- No `TypeSpec::tag`, HIR struct layout identity, codegen type declaration
  name, struct completeness, member lookup, static-member lookup, or method
  owner migration.
- No parser struct/tag map, template rendered-name, or `TypeSpec::tag` output
  cleanup.
- No deletion of string maps before stable dual-lookup proof.
- No expectation downgrades, unsupported-test demotions, or named-case
  shortcuts.

## Working Model

- Prefer structured or `TextId` helper variants for parser-owned call sites
  when stable identity is already present.
- Keep string helper overloads as explicit compatibility bridges while
  downstream callers still provide only rendered names.
- Carry parser-owned `TextId` metadata through AST fields when sema mirrors need
  definition-time identity that cannot be recovered from rendered strings.
- For template parameters, treat the AST parameter array as the identity source:
  mirror work may use `TextId` only after `Node` carries metadata parallel to
  `template_param_names`; otherwise it must skip structured writes and keep the
  rendered-name path as the compatibility bridge.
- Dual-write sema mirrors beside existing rendered maps where AST/parser
  metadata supplies stable identity.
- Dual-read and compare structured and legacy results before changing behavior.
- Treat rendered names as required bridge output for HIR, diagnostics,
  consteval, codegen, ABI/link-visible names, and template spelling.
- Resolve unused mirror plumbing by proving it meaningful and populating it, or
  by deleting it as dead scaffolding with focused proof.

## Execution Rules

- Keep each implementation step behavior-preserving unless the step explicitly
  reaches a proof-backed fallback demotion.
- Update `todo.md` with packet progress and proof results; do not rewrite this
  plan for routine packet completion.
- For each code-changing step, run `cmake --build --preset default` plus the
  focused frontend/parser/sema CTest subset selected by the supervisor.
- For enum, consteval, or template-binding steps that cross parser/sema
  boundaries, use matching before/after logs for the same focused CTest command
  when requested by the supervisor.
- Escalate to regression guard or broader CTest only when the touched path
  reaches HIR, compile-time engine behavior, emitted codegen, or baseline drift
  risk.
- Do not weaken supported-path tests or hide mismatches through expectation
  rewrites.

## Ordered Steps

### Step 1: Inventory Parser/Sema Leftover Callers

Goal: confirm the exact parser/sema-owned caller surfaces before changing any
lookup behavior.

Primary targets:
- `review/97_structured_identity_completion_audit.md`
- `src/frontend/parser/`
- `src/frontend/impl/`
- `src/frontend/sema/`

Actions:
- Re-read the idea 97 audit classifications for `parser-leftover` and
  `sema-leftover` entries.
- Inventory parser helper overload callers for const-int evaluation,
  typedef-chain resolution, and type compatibility.
- Separate parser-owned call sites with stable `TextId` from downstream
  string-only bridge callers.
- Inventory sema enum variant, template NTTP, template type-parameter,
  consteval NTTP binding, and type-binding mirror surfaces.
- Record any caller that must remain string-only because it is bridge-required
  or blocked by HIR/type identity.

Completion check:
- `todo.md` names the inspected surfaces and the first implementation packet.
- No implementation files or tests are modified in this inventory-only step.

### Step 2: Prefer Structured Parser Helper Inputs

Goal: move parser-owned helper callers to structured or `TextId` inputs where
stable identity is already available.

Primary targets:
- `src/frontend/parser/parser_support.hpp`
- `src/frontend/impl/support.cpp`
- Parser-owned call sites found in Step 1

Actions:
- Prefer existing `TextId` const-int helper variants for parser-owned callers
  that already have stable IDs.
- Add or tighten paired legacy comparison only where structured and rendered
  inputs are both available.
- Keep string helper overloads documented or shaped as compatibility bridges
  for downstream string-only callers.
- Do not touch parser struct/tag maps, template rendered names, or
  `TypeSpec::tag` outputs.

Completion check:
- Parser-owned callers prefer structured identity where available.
- String overloads remain only as explicit compatibility bridges or proven
  legacy proof paths.
- Focused parser proof covers structured and legacy fallback behavior without
  expectation downgrades.

### Step 3A: Add AST/Parser Enumerator TextId Metadata

Goal: make definition-time enumerator identity available to sema without
deriving it from rendered strings.

Primary targets:
- `src/frontend/parser/ast.hpp`
- `src/frontend/parser/impl/types/struct.cpp`
- Parser enum tests selected by the supervisor

Actions:
- Add a parallel per-enumerator `TextId` metadata field to `NK_ENUM_DEF` while
  preserving existing rendered `enum_names`, `enum_vals`, and
  `enum_has_explicit` bridge fields.
- Store the `vname_text_id` already computed during enum parsing into the new
  metadata array for each enumerator.
- Keep duplicate detection and enum-initializer evaluation based on the
  existing parser `TextId` path; do not re-derive stable identity from rendered
  strings after the parser has consumed the token.
- Ensure enum references, empty enum definitions, and variants without a valid
  token `TextId` leave the metadata absent or `kInvalidText` in a way sema can
  safely skip.
- Do not change HIR enum collection, consteval registration, codegen naming, or
  diagnostic rendering.

Completion check:
- `NK_ENUM_DEF` exposes stable per-enumerator `TextId` metadata beside the
  existing rendered name/value arrays.
- Focused parser/build proof passes without changing enum rendering or HIR
  bridge behavior.
- `todo.md` points the next packet at Step 3B.

### Step 3B: Populate Sema Enum Variant Mirrors

Goal: populate enum variant text/key mirrors for global and local sema bindings
where stable AST identity exists.

Primary targets:
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`

Actions:
- Identify enum variant binding registration paths for global, local,
  namespace, and record-adjacent contexts.
- Read the per-enumerator `TextId` metadata added by Step 3A; skip structured
  mirror writes when an enumerator lacks a valid stable ID.
- Dual-write text/key mirrors beside existing rendered enum maps when the AST
  exposes stable identity, including `structured_enum_consts_`,
  `enum_const_vals_global_by_text_`, `enum_const_vals_global_by_key_`,
  `enum_const_vals_scopes_by_text_`, and
  `enum_const_vals_scopes_by_key_` where the required key metadata exists.
- Dual-read or compare lookup results where both structured and rendered
  bindings are available.
- Preserve rendered maps for diagnostics, consteval, HIR, and compatibility
  callers.

Completion check:
- Enum variant mirrors are populated for supported global and local bindings.
- Focused proof covers enum constants in the relevant scopes and shows no
  behavior drift.

### Step 4A: Add AST/Parser Template Parameter TextId Metadata

Goal: make definition-time template parameter identity available to sema
without deriving it from rendered parameter names.

Primary targets:
- `src/frontend/parser/ast.hpp`
- `src/frontend/parser/impl/declarations.cpp`
- Parser template tests selected by the supervisor

Actions:
- Add a parallel per-parameter `TextId` metadata field to `Node` beside
  `template_param_names`, preserving the existing rendered names, NTTP flags,
  pack flags, defaults, template arguments, and bridge behavior.
- Capture the parser token `TextId` for each template parameter when the
  template parameter list is parsed; store `kInvalidText` for synthetic,
  copied, instantiated, or otherwise unsupported parameter metadata that cannot
  claim stable definition-time identity.
- Populate the metadata for every declaration/reference path that currently
  receives `template_param_names`, keeping array length and index alignment
  exactly parallel to `n_template_params`.
- Do not change template rendered names, mangling, canonical symbol output,
  HIR-facing type tags, diagnostics, consteval evaluation, codegen, or
  link-visible names.
- Leave sema mirror writes for Step 4B; this step only creates parser/AST
  identity metadata and preserves current behavior.

Completion check:
- `Node` exposes stable per-template-parameter `TextId` metadata beside
  `template_param_names` where the parser has a real parameter token.
- Existing parser/template behavior is unchanged when metadata is invalid or
  absent.
- Focused parser/build proof passes without changing template rendering or HIR
  bridge behavior.
- `todo.md` points the next packet at Step 4B.

### Step 4B: Mirror Template NTTP Validation Bindings

Goal: add structured or `TextId` mirrors for sema-owned template NTTP
placeholders and validation bindings using the parser/AST parameter identity
metadata from Step 4A.

Primary targets:
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/consteval.cpp`
- `src/frontend/sema/consteval.hpp`

Actions:
- Locate validation local scopes and NTTP placeholder registrations that remain
  name-map only.
- Read the per-parameter `TextId` metadata added by Step 4A; skip structured
  mirror writes when a parameter lacks a valid stable ID.
- Dual-write NTTP placeholder mirrors keyed by stable parameter identity where
  metadata is available, preserving existing local rendered-name bindings for
  compatibility.
- Add dual-read lookup or mismatch proof without changing behavior for callers
  that still provide only rendered names.
- Keep fallback behavior for template metadata that lacks stable text IDs.

Completion check:
- Template NTTP validation has structured or `TextId` mirrors where source
  metadata supports them.
- Focused template/consteval proof passes without weakening expectations.

### Step 5: Mirror Template Type-Parameter Validation

Goal: replace name-set-only sema checks with structured or `TextId` mirrors
using the parser/AST parameter identity metadata from Step 4A where template
type-parameter identity is available.

Primary targets:
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/type_utils.cpp`
- `src/frontend/sema/type_utils.hpp`

Actions:
- Inventory name-set based type-parameter tracking in validation and cast/type
  checks.
- Read the per-parameter `TextId` metadata added by Step 4A; skip structured
  mirror writes or checks when a type parameter lacks a valid stable ID.
- Add structured or `TextId` mirrors for sema-owned checks where stable
  parameter identity exists, preserving rendered type-parameter names as the
  compatibility bridge.
- Compare structured and rendered decisions where both inputs are available.
- Preserve HIR-facing type tags and rendered type names.

Completion check:
- Template type-parameter validation can use structured identity where
  metadata supports it.
- Focused proof covers type-parameter checks and cast validation without HIR
  data-model changes.

### Step 6: Populate Consteval NTTP Binding Mirrors

Goal: populate consteval call NTTP binding text/key maps and dual-read lookup
where call metadata and the parser/AST parameter identity metadata from Step 4A
support it.

Primary targets:
- `src/frontend/sema/consteval.cpp`
- `src/frontend/sema/consteval.hpp`
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`

Actions:
- Identify consteval call NTTP binding creation for explicit and default NTTP
  values.
- Read stable template parameter `TextId` metadata when binding values back to
  definition parameters; skip text/key map writes when either template or
  parameter identity is unavailable.
- Populate text/key maps beside the existing legacy name map when metadata is
  available.
- Add dual-read lookup or mismatch proof against the legacy name map.
- Keep rendered binding maps for compatibility with consteval and downstream
  bridge consumers.

Completion check:
- Consteval NTTP binding lookup populates and checks text/key mirrors when
  metadata is available.
- Focused consteval proof passes for explicit and default NTTP values.

### Step 7: Resolve Type-Binding Text Mirror Plumbing

Goal: make type-binding text mirrors meaningful or remove them as proven-dead
compatibility scaffolding.

Primary targets:
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/type_utils.cpp`
- `src/frontend/sema/type_utils.hpp`
- Any consteval/template substitution path found in Step 1

Actions:
- Trace declared and threaded type-binding text mirror outputs to their
  consumers.
- If the mirror supports a sema-owned lookup path, populate and check it beside
  the rendered map.
- If source evidence proves the mirror is dead scaffolding, remove it without
  changing external behavior.
- Do not change HIR-facing type tags or rendered type names.

Completion check:
- Type-binding text mirror plumbing is either populated and proven useful, or
  removed as dead scaffolding with focused proof.
- Template substitution and consteval/type-binding proof passes.

### Step 8: Demote Only Proven Parser/Sema Legacy Fallbacks

Goal: remove or demote parser/sema-owned legacy fallbacks only after focused
proof shows stable structured parity.

Primary targets:
- Parser/sema fallback sites changed by Steps 2-7
- Focused frontend/parser/sema tests selected by the supervisor

Actions:
- Review mismatch evidence and proof results from prior steps.
- Demote only fallbacks that are parser/sema-owned and not bridge-required.
- Keep rendered maps and fallback paths required by AST, HIR, consteval,
  diagnostics, codegen, link names, or unsupported metadata.
- Run the supervisor-selected broader proof if multiple narrow-only packets
  have accumulated.

Completion check:
- Remaining string fallbacks are either explicit compatibility bridges,
  diagnostic/rendering output, HIR/type blocked, or legacy proof still needed.
- Focused parser/sema proof passes with no expectation downgrades.
- No HIR/type/codegen rendered-name bridge behavior changes.
