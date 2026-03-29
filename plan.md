# SFINAE Template-Pattern Parsing and Staged Support Runbook

Status: Active
Source Idea: ideas/open/06_sfinae_template_parameter_and_signature_patterns.md
Activated from: prompts/AGENT_PROMPT_ACTIVATE_PLAN.md

## Purpose

Turn the open SFINAE parser idea into an execution runbook that drives small,
test-first parser slices without silently expanding into full template
semantics.

## Goal

Make the parser reliably accept the common libstdc++ / stdlib SFINAE surface
forms that appear in template parameter lists and function signatures while
keeping unsupported semantics localized and explicit.

## Core Rule

Treat the current work as parser-coverage and AST-stability work first. Do not
claim full SFINAE semantics, overload pruning, or template partial ordering as
part of this plan.

## Read First

- [ideas/open/06_sfinae_template_parameter_and_signature_patterns.md](/workspaces/c4c/ideas/open/06_sfinae_template_parameter_and_signature_patterns.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- [src/frontend/parser/parser.h](/workspaces/c4c/src/frontend/parser/parser.h)
- [tests/cpp/internal/postive_case](/workspaces/c4c/tests/cpp/internal/postive_case)

## Current Targets

The first support wave is limited to these six pattern families:

1. unnamed typed NTTP defaults in `template<...>`
2. named typed NTTP defaults in `template<...>`
3. unnamed type parameters defaulted through `enable_if`
4. return-type SFINAE
5. parameter-type SFINAE
6. partial specialization or alias-specialization gating through `enable_if`

## Non-Goals

- real substitution-failure behavior
- overload-set pruning via SFINAE
- broad alias-template semantic completion
- full template partial ordering
- container-specific debugging unrelated to these parser forms

## Working Model

- Parse SFINAE-shaped declarations as ordinary syntax first.
- Reuse shared parsing paths between top-level and record-member template
  preludes instead of introducing ad hoc special cases.
- Preserve enough AST structure for later semantic work.
- If a parsed construct reaches an unsupported semantic phase, fail at that
  construct with a focused error rather than via downstream parser corruption.

## Execution Rules

- Follow a test-first workflow for each pattern family.
- Add the narrowest reduced parser regression before or with the parser change.
- Keep each patch to one mechanism family.
- When a failure is semantic rather than syntactic, record it explicitly and do
  not widen parser recovery rules to hide it.
- If execution uncovers a separate initiative, write it to `ideas/open/`
  instead of silently growing this plan.

## Ordered Steps

### Step 1: Capture and reduce the active failure shapes

Goal:
- map the six target families onto concrete reduced parse tests and note which
  ones already fail in tree

Primary target:
- [tests/cpp/internal/postive_case](/workspaces/c4c/tests/cpp/internal/postive_case)

Actions:
- inspect existing parser tests for overlapping `enable_if` or dependent
  template-parameter coverage
- add missing reduced tests for each target family, marking any not-yet-fixed
  cases as expected failures only if the harness requires it
- record the exact parser failure signature for each missing family

Completion check:
- each of the six target families has a named reduced testcase or an explicit
  recorded reason it is blocked from being added immediately

### Step 2: Normalize template-parameter-list parsing for SFINAE forms

Goal:
- accept `enable_if`-shaped type heads and typed NTTP defaults inside
  `template<...>` without downstream parse-state corruption

Primary target:
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)

Actions:
- factor shared type-head and NTTP handling used by top-level and record-member
  template preludes
- ensure alias, qualified, and `typename ...::type` spellings are accepted in
  template parameter context
- validate unnamed and named typed NTTP cases with focused tests

Completion check:
- the template-parameter target families parse cleanly in both reduced tests
  and any nearby existing parser regressions

### Step 3: Normalize SFINAE-bearing function signatures

Goal:
- parse return-type and parameter-type SFINAE forms through the normal function
  declaration pipeline

Primary targets:
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- any adjacent function/declarator helpers touched by the narrow fix

Actions:
- support trailing return and parameter-type `enable_if` spellings as ordinary
  type syntax
- keep declarator parsing layered so signature fixes do not become general
  recovery widening
- confirm top-level and record-member function declarations share the same
  support path where applicable

Completion check:
- reduced return-type and parameter-type SFINAE tests pass without creating new
  parser regressions in nearby declarator coverage

### Step 4: Cover specialization-gating forms and stage semantic fallout

Goal:
- parse partial specialization and alias-specialization gating forms while
  keeping unsupported semantics localized

Primary targets:
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- semantic diagnostics reached by the newly parsed forms, if any

Actions:
- add reduced specialization-gating tests
- implement only the minimum parser support needed to preserve structure
- if sema or lowering lacks support, emit or preserve a focused failure at the
  unsupported construct rather than allowing later parser corruption

Completion check:
- specialization-gating tests parse successfully, or they fail later with a
  specific localized unsupported-feature signal

### Step 5: Validate the slice with regression guardrails

Goal:
- prove the active slice improves or preserves suite health without new
  unrelated failures

Actions:
- run the targeted reduced tests for the just-finished mechanism family
- run nearby parser tests in the same subsystem
- run the full configured `ctest` suite before handoff
- compare before/after full-suite logs and keep the result monotonic

Completion check:
- targeted tests pass for the completed slice
- full-suite results are monotonic with no newly failing tests
- `plan_todo.md` records what completed and what should be tackled next
