# Parser Known Function Name Compatibility Spelling Cleanup Runbook

Status: Active
Source Idea: ideas/open/137_parser_known_function_name_compatibility_spelling_cleanup.md

## Purpose

Retire parser known-function-name compatibility spelling paths where rendered
function names can still decide declaration and call disambiguation after
`QualifiedNameKey` identity is available.

## Goal

Make structured parser name identity the normal semantic authority for known
function-name registration and lookup, keeping rendered spellings only for
diagnostics, final spelling, or explicit compatibility fallback.

## Core Rule

Prefer `QualifiedNameKey`, `TextId`, parser name paths, and namespace context
ids over reparsed rendered strings whenever structured identity is available.

## Read First

- `ideas/open/137_parser_known_function_name_compatibility_spelling_cleanup.md`
- Parser known-function-name storage and helpers around `known_fn_names`
- Declaration parsing paths for namespace-qualified functions, operators, and
  constructors
- Expression/type disambiguation paths that probe known function names

## Current Targets

- `has_known_fn_name(const std::string&)`
- `register_known_fn_name(const std::string&)`
- `register_known_fn_name_in_context()` rendered-spelling fallback branches
- Registration call sites for `qualified_op_name`, `qualified_ctor_name`, and
  `scoped_decl_name`
- Call-disambiguation probes using rendered `head_name`,
  `current_member_name`, or visible-name spelling

## Non-Goals

- Do not change function declaration syntax, overload semantics, or namespace
  visibility rules.
- Do not remove final AST function spelling or generated operator/constructor
  names before downstream consumers have structured replacements.
- Do not fold this work into Sema/HIR owner or member lookup cleanup.
- Do not broaden into parser string cleanup outside known-function-name
  registration and disambiguation.

## Working Model

- `known_fn_names` should be keyed by structured `QualifiedNameKey` identity.
- Rendered function spelling is acceptable for diagnostics, AST/final text, and
  documented fallback only.
- If a structured key exists, rendered spelling must not silently override or
  decide disambiguation.
- Any retained compatibility fallback should be visible enough to diagnose
  mismatches between structured and rendered identities.

## Execution Rules

- Keep each code slice behavior-preserving except where the source idea
  explicitly requires removing semantic authority from rendered names.
- Replace rendered-name producers and consumers with structured helper paths
  before deleting compatibility overloads.
- Avoid testcase-shaped shortcuts; prove same-feature behavior around
  namespace-qualified functions, operators, constructors, and visible aliases.
- Each code-changing step needs fresh build or compile proof plus the focused
  parser/frontend tests relevant to the touched path.

## Ordered Steps

### Step 1: Map Structured and Rendered Known-Name Paths

Goal: identify every producer and consumer that still crosses through rendered
function-name spelling for known-function-name authority.

Actions:
- Inspect `known_fn_names` helpers and overloads.
- Trace registration from declaration parsing, namespace/context qualification,
  operators, constructors, and scoped declarations.
- Trace lookup from expression/type disambiguation and visible-name probes.
- Record any compatibility-only spelling paths in `todo.md` rather than
  expanding the plan.

Completion check:
- The executor can name the remaining rendered-spelling authority paths and
  the first structured replacement target without touching source intent.

### Step 2: Add Structured Registration Paths

Goal: make declaration-side known-function-name registration use structured
identity as the primary path.

Actions:
- Introduce or reuse structured registration helpers for
  `QualifiedNameKey`/context-derived identity.
- Convert registration call sites for namespace-qualified functions,
  operators, constructors, and scoped declarations away from rendered spelling
  when structured identity is available.
- Keep rendered spelling only as explicit fallback or final/display data.

Completion check:
- Registration for free functions, namespace-qualified functions, out-of-class
  operators, and out-of-class constructors reaches `known_fn_names` through
  structured keys.
- Focused build/test proof covers the converted registration paths.

### Step 3: Add Structured Lookup and Disambiguation Paths

Goal: make parser disambiguation consult structured known-function-name keys
before any compatibility spelling probe.

Actions:
- Convert lookup sites that use rendered `head_name`, `current_member_name`, or
  visible-name spelling to structured keys where the parser already has enough
  identity.
- Keep string lookup paths compatibility-only and structured-primary.
- Add mismatch or fallback visibility where rendered spelling remains.

Completion check:
- Declaration/call disambiguation no longer depends on rendered spelling when
  structured identity is available.
- Focused parser/frontend tests cover visible-name and namespace-qualified
  disambiguation.

### Step 4: Quarantine or Remove String Compatibility Overloads

Goal: ensure remaining string overloads cannot silently decide semantic
known-function-name identity.

Actions:
- Remove string overloads whose call sites have structured replacements.
- For required compatibility callers, narrow overload names or comments so the
  fallback contract is explicit.
- Verify that rendered-name reparsing is absent from normal semantic paths.

Completion check:
- Any remaining rendered-name overloads are compatibility-only and not used as
  the normal declaration or call-disambiguation path.
- Focused proof covers both structured-present and compatibility-fallback
  behavior where fallback is still supported.

### Step 5: Broader Parser Confidence Check

Goal: prove the cleanup generalizes beyond one named testcase.

Actions:
- Run the focused parser/frontend tests for namespace-qualified functions,
  out-of-class operators, out-of-class constructors, and visible-name
  disambiguation.
- Run the supervisor-selected broader validation checkpoint for the parser
  bucket before treating the idea as closure-ready.
- Leave any adjacent non-known-function-name parser string cleanup as a
  separate open idea instead of absorbing it here.

Completion check:
- Acceptance criteria from the source idea are satisfied with structured
  known-function-name authority and focused same-feature coverage.
