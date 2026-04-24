# Parser Public Facade PIMPL Boundary Runbook

Status: Active
Source Idea: ideas/open/94_parser_public_facade_pimpl_boundary.md

## Purpose

Make `src/frontend/parser/parser.hpp` a genuinely small public facade by
moving parser-private state ownership and implementation-only method
declarations behind an implementation boundary.

Goal: external callers can construct `Parser`, configure public parser
options/debug controls, call `parse()`, and inspect the supported public
result/error surface without depending on private parser state carrier
definitions.

## Core Rule

`parser.hpp` is the public facade; `src/frontend/parser/impl/parser_impl.hpp`
is the private parser implementation index.

## Read First

- `ideas/open/94_parser_public_facade_pimpl_boundary.md`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/parser_impl.hpp`
- `src/frontend/parser/impl/parser_state.hpp`
- parser implementation files under `src/frontend/parser/`
- current parser-focused tests under `tests/`

## Current Targets

- Public facade:
  - `src/frontend/parser/parser.hpp`
- Private implementation boundary:
  - `src/frontend/parser/impl/parser_impl.hpp`
  - `src/frontend/parser/impl/parser_state.hpp`
- Split parser implementation files that currently implement `Parser` methods
  directly over public-header state fields.
- Parser caller and test include sites that should remain source-compatible
  with the public facade.

## Non-Goals

- Do not change parser semantics.
- Do not change AST output, diagnostics behavior, pragma behavior, template
  parsing behavior, lexical-scope behavior, or testcase expectations.
- Do not downgrade supported-path tests or weaken contracts.
- Do not split the AST model.
- Do not create a traditional separated `include/` tree.
- Do not introduce one header per parser `.cpp`.
- Do not do broad parser algorithm rewrites beyond what the facade boundary
  requires.
- Do not move unrelated frontend, HIR, BIR, LIR, or codegen boundaries.

## Working Model

The preferred ownership shape is:

```text
src/frontend/parser/parser.hpp
  public Parser facade, public options/debug controls, parse result/error API,
  and opaque implementation ownership

src/frontend/parser/impl/parser_impl.hpp
  private implementation index used by parser `.cpp` files

src/frontend/parser/impl/parser_state.hpp
  complete parser-private state carriers used only behind the implementation
  boundary
```

The first successful structural outcome is removing the private state include
from `parser.hpp` while preserving normal public parser construction and
`parse()` behavior.

## Execution Rules

- Keep each step behavior-preserving.
- Prefer one narrow buildable slice at a time: inspect, introduce the opaque
  boundary, migrate implementation methods, then tighten the public facade.
- Keep test-only hooks clearly named and out of normal external caller
  dependency paths.
- Do not add testcase-shaped shortcuts or named-case special handling.
- After each code-changing step, run at least:
  - `cmake --build build -j --target c4c_frontend c4cll`
  - `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
- Escalate validation if a slice changes AST-facing contracts, diagnostics
  formatting, HIR-facing parser outputs, or build-system include visibility.

## Step 1: Inventory Public Parser Surface

Goal: identify which declarations in `parser.hpp` are true public API and
which are implementation-only declarations that can move behind the private
boundary.

Primary targets:
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/parser_impl.hpp`
- parser callers in frontend app, HIR, parser tests, and command-line paths

Actions:
- Inspect external includes and direct uses of `Parser`, parser options/debug
  controls, `parse()`, parse results, diagnostics, and test-only hooks.
- List `Parser` members and methods that are private implementation surface
  because they operate directly on parser state carriers.
- Decide the minimal public destructor, move constructor, or ownership
  declarations needed if `Parser` stores an opaque implementation object.
- Record any caller dependency that is intentionally test-only instead of
  exposing private state through normal `parser.hpp` users.

Completion check:
- Public parser API and private implementation surface are separated in
  `todo.md`.
- No implementation files are changed in this step unless required for a
  compile-proofing probe.

## Step 2: Introduce Opaque Parser Implementation Ownership

Goal: make `Parser` own parser-private state through an opaque implementation
object without changing public construction or `parse()` behavior.

Primary targets:
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/parser_impl.hpp`
- `src/frontend/parser/impl/parser_state.hpp`
- parser constructor/destructor implementation file

Actions:
- Add an implementation-owned object such as `ParserImpl` or an equivalent
  private state wrapper, with complete type visibility through
  `impl/parser_impl.hpp`.
- Replace value-owned private state carriers in `Parser` with opaque ownership
  in the public class layout.
- Define special members where the complete implementation type is visible.
- Keep the public constructor and public `parse()` entry behavior stable.
- Avoid exposing `impl/parser_state.hpp` through the public header.

Completion check:
- `parser.hpp` no longer needs complete private state carrier types for the
  `Parser` object layout.
- `c4c_frontend`, `c4cll`, and focused parser tests build/pass.

## Step 3: Move Implementation-Only Method Declarations Behind The Boundary

Goal: remove split-translation-unit parser implementation declarations from
the public facade where practical.

Primary targets:
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/parser_impl.hpp`
- parser `.cpp` files that define or call private parsing helpers

Actions:
- Move implementation-only helper declarations that require private state into
  the private implementation surface.
- Retarget parser `.cpp` files to include `impl/parser_impl.hpp` when they need
  private parser state or helper declarations.
- Keep public `Parser` declarations limited to construction, configuration,
  `parse()`, and supported result/error inspection.
- Preserve test-only hook names when they must remain reachable, and keep them
  visibly marked as test-only.

Completion check:
- Parser implementation files have one obvious private index to open:
  `src/frontend/parser/impl/parser_impl.hpp`.
- External callers of `parser.hpp` do not see private parser state carrier
  definitions.
- Focused parser proof is recorded in `todo.md`.

## Step 4: Retarget Callers And Test Hooks

Goal: make all normal callers depend only on the public facade while keeping
needed parser tests explicit about private or test-only access.

Primary targets:
- app/frontend/parser caller include sites
- HIR or AST paths that include parser headers
- parser tests and parser test support helpers

Actions:
- Update normal callers to include only `parser.hpp` unless they are parser
  implementation files.
- Move private implementation includes out of app, HIR, and normal frontend
  caller paths.
- Keep any test-only private access clearly named and isolated in test support
  code.
- Avoid weakening tests to fit the new boundary.

Completion check:
- External app, HIR, frontend, and parser-test callers can include
  `parser.hpp` without seeing private parser state carrier definitions.
- Test-only private access is explicit and does not become a normal public API.
- `c4c_frontend`, `c4cll`, and focused parser tests build/pass.

## Step 5: Tighten Public Header And Include Proof

Goal: prove the facade boundary is real and not only hidden by incidental
include order.

Primary targets:
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/parser_impl.hpp`
- build dependency or include-order-sensitive parser tests

Actions:
- Confirm `parser.hpp` does not include
  `src/frontend/parser/impl/parser_state.hpp`.
- Remove accidental public transitives that make private state visible through
  the facade.
- Add or adjust a focused include/build proof only if existing tests do not
  catch public-header leakage.
- Keep the proof about header boundary, not parser semantic behavior.

Completion check:
- `parser.hpp` is self-sufficient for public parser callers.
- Private parser state carriers are only visible behind the implementation
  boundary or explicit test-only hooks.
- Focused parser proof is recorded in `todo.md`.

## Step 6: Final Parser Facade Validation

Goal: prove the source idea acceptance criteria are satisfied and ready for
plan-owner closure review.

Actions:
- Re-check each acceptance criterion from
  `ideas/open/94_parser_public_facade_pimpl_boundary.md`.
- Run:
  - `cmake --build build -j --target c4c_frontend c4cll`
  - `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
- Escalate to broader frontend or full CTest if include movement crossed AST,
  HIR, diagnostics, CLI, or build-system boundaries.
- Record any durable remaining parser-boundary work as a separate open idea
  instead of silently expanding this runbook.

Completion check:
- Source-idea acceptance criteria are satisfied or explicit follow-up scope is
  recorded separately.
- Public parser behavior remains source-compatible for normal construction,
  debug configuration, `parse()`, and parse-error inspection.
- The active runbook can be considered for closure only after the source idea
  itself is complete.
