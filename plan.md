# NTTP Type Binding Domain Key Contract Runbook

Status: Active
Source Idea: ideas/open/150_nttp_type_binding_domain_key_contract.md

## Purpose

Replace string-pair template binding metadata in deferred Parser/HIR evaluation APIs with structured `TextId` plus parameter-domain binding metadata for type parameters and non-type template parameters.

Goal: make deferred NTTP/type binding and HIR late substitution use structured parameter-domain identity instead of rendered parameter-name strings, token spelling comparison, or `TemplateArgRef::debug_text`.

## Core Rule

`TextId` is spelling identity only. Semantic template parameter binding identity must include domain metadata such as owner template key, parameter index, parameter kind, and structured binding payload.

## Read First

- `ideas/open/150_nttp_type_binding_domain_key_contract.md`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/hir/impl/templates/type_resolution.cpp`
- `src/frontend/hir/impl/templates/materialization.cpp`
- `src/frontend/hir/hir_functions.cpp`
- `src/frontend/hir/compile_time_engine.hpp`

## Current Targets

- Deferred parser APIs around `eval_deferred_nttp_default`, `eval_deferred_nttp_expr_tokens`, and `eval_captured_template_arg_expr_tokens`.
- Parser token paths that bind values by parameter-name string or token spelling.
- HIR late substitution paths using `type_bindings.find(string)`, `nttp_bindings.find(string)`, or `TemplateArgRef::debug_text`.
- Compatibility adapters for legacy `vector<pair<string, TypeSpec>>`, `vector<pair<string, long long>>`, and HIR string binding maps.

## Non-Goals

- Do not redesign the full constant evaluator.
- Do not force all NTTP/type binding resolution to finish in Sema before HIR.
- Do not treat `TextId` alone as semantic template parameter identity.
- Do not rewrite diagnostics, dumps, or source spelling renderers except where needed to keep behavior compiling.
- Do not weaken tests or convert supported paths to unsupported.
- Do not add named-fixture or testcase-shaped shortcuts.

## Working Model

- Parser preserves template parameter and argument syntax carriers, including spelling as `TextId`, order, kind, owner context when known, and structured payloads.
- Sema assigns parameter-domain identity where possible and produces structured deferred binding carriers for dependent defaults and captured expressions.
- HIR consumes the structured carriers and performs late substitution under the current instantiation context.
- Legacy string-keyed forms may remain temporarily only as compatibility wrappers that convert into the structured contract.

## Execution Rules

- Keep each step behavior-preserving unless the step explicitly swaps semantic lookup authority.
- Prefer adding typed carriers and conversion boundaries before removing legacy call shapes.
- When retaining string-keyed code, label or isolate it as compatibility/display/syntax behavior, not semantic authority.
- For every code-changing step, run a fresh build or narrower compile proof chosen by the supervisor.
- Escalate to broader validation when a step touches shared template substitution or compile-time evaluation behavior.

## Step 1: Inventory String Binding Authority

Goal: identify every current semantic lookup path that depends on rendered parameter-name strings, token spelling, string-pair vectors, or `TemplateArgRef::debug_text`.

Primary targets:
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/hir/impl/templates/type_resolution.cpp`
- `src/frontend/hir/impl/templates/materialization.cpp`
- `src/frontend/hir/hir_functions.cpp`
- `src/frontend/hir/compile_time_engine.hpp`

Actions:
- Inspect parser deferred NTTP/type evaluation declarations and call sites.
- Inspect parser implementation for token-spelling comparisons used to bind parameter values.
- Inspect HIR late binding and substitution maps keyed by `std::string`.
- Classify each finding as semantic authority, compatibility adapter, syntax/display behavior, or unrelated.
- Record the implementation packet boundary and proof command in `todo.md` before code changes begin.

Completion check:
- `todo.md` names the first concrete implementation packet, the files it owns, and the exact proof command delegated by the supervisor.
- No source idea edits are needed unless the inventory contradicts durable source intent.

## Step 2: Define Structured Binding Carriers

Goal: introduce typed metadata for type parameter bindings and NTTP parameter bindings without broad call-site churn.

Actions:
- Define carriers that include parameter spelling `TextId`, parameter kind, parameter index, owner template key/context when available, and structured type/value/deferred payload.
- Keep type-parameter and NTTP-value bindings distinct.
- Add compatibility constructors or conversion helpers from legacy string-pair vectors only where needed to keep existing callers compiling.
- Document in code comments only where necessary that compatibility conversion is not semantic authority.

Completion check:
- New structured carriers compile.
- Legacy string-pair inputs convert into the structured contract at clear boundaries.
- No new semantic lookup depends on `std::string` parameter names or `TextId` alone.

## Step 3: Migrate Parser Deferred Evaluation APIs

Goal: make parser deferred NTTP and captured argument evaluation pass structured binding metadata instead of authoritative string-pair bindings.

Actions:
- Update `eval_deferred_nttp_default`, `eval_deferred_nttp_expr_tokens`, and `eval_captured_template_arg_expr_tokens` declarations and call sites as narrowly as possible.
- Replace token-spelling parameter binding with lookup through structured binding metadata.
- Preserve deferred evaluation behavior; do not require Sema to resolve every dependent value eagerly.
- Keep legacy wrappers only where existing callers cannot migrate in the same packet.

Completion check:
- Parser paths compile and no longer bind semantic NTTP values primarily through `token_spelling(tok) == parameter_name`.
- Compatibility wrappers, if retained, feed structured bindings before evaluation.

## Step 4: Migrate HIR Late Substitution

Goal: make HIR late type and NTTP substitution consume structured binding carriers instead of rendered string keys.

Primary targets:
- `src/frontend/hir/impl/templates/type_resolution.cpp`
- `src/frontend/hir/impl/templates/materialization.cpp`
- `src/frontend/hir/hir_functions.cpp`
- `src/frontend/hir/compile_time_engine.hpp`

Actions:
- Replace authoritative `type_bindings.find(string)` and `nttp_bindings.find(string)` lookups with parameter-domain binding lookup.
- Remove semantic dependence on `TemplateArgRef::debug_text`.
- Preserve string rendering only for diagnostics, dump output, syntax compatibility, or transitional adapters.
- Keep type and NTTP binding domains separate through substitution.

Completion check:
- HIR late substitution compiles and uses structured binding metadata as its primary route.
- Any remaining string maps are compatibility-only and have a clear removal condition.

## Step 5: Add Identity-Focused Tests Or Probes

Goal: prove the new authority handles a case where spelling-only matching is wrong or insufficient.

Actions:
- Add or update focused tests for equal parameter spelling in different template contexts, captured argument expressions, formatting differences, or another nearby case that would fail under string-pair authority.
- Avoid expectation downgrades and unsupported conversions.
- Include at least one negative reject signal in review notes if a known stringly route remains intentionally temporary.

Completion check:
- The targeted test or probe fails under the old spelling-only authority and passes through structured binding lookup.
- Nearby same-feature cases are considered enough to avoid a named-case-only fix.

## Step 6: Validate And Clean Compatibility Boundaries

Goal: ensure the route satisfies the source idea without hiding the old failure mode behind new names.

Actions:
- Re-run the supervisor-selected narrow proof and any broader validation required by the touched template/HIR surfaces.
- Inventory remaining string-pair vectors, string maps, token-spelling comparisons, and `debug_text` uses touched by this idea.
- Classify remaining uses as compatibility/display/syntax behavior or open follow-up.
- Remove obsolete adapters when all active call sites use structured carriers.

Completion check:
- Acceptance criteria in the source idea are satisfied or remaining work is explicitly recorded in `todo.md`.
- Regression proof is fresh.
- Reviewer reject signals have been checked, especially overfit, expectation downgrades, `TextId`-alone identity, and renamed stringly authority.
