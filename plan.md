# c4 Preprocessor Execution Plan

Last updated: 2026-03-13

This file is the execution-oriented plan for the preprocessor track.
Source analysis lives in [`preprocessor_plan.md`](preprocessor_plan.md); this file is the implementation queue that agents should follow.

## Goal

Turn the internal preprocessor into the default, reliable path for frontend work, with Clang-compatible behavior for:

1. output contract
2. include handling
3. pragma handling
4. `#if` evaluation
5. macro expansion

The main strategy is:

1. stabilize structure first
2. make internal behavior deterministic
3. reduce external fallback to last-resort only

## Priorities

Work in this order unless a later item is required to unblock the current slice:

1. Phase 0: structure refactor
2. Phase 1: output contract and public API
3. Phase 2: include system
4. Phase 3: pragma system
5. Phase 4: `#if` expression and intrinsics
6. Phase 5: macro expansion completeness
7. Phase 6: predefined macros and target configuration
8. Phase 7: builtin header injection and fallback declarations

## Global Rules

1. Prefer small, shippable slices over large rewrites.
2. Keep behavior monotonic: no previously passing test may regress.
3. Add or update focused tests before implementing a new behavior.
4. Compare internal behavior against `clang -E` or `clang -S -emit-llvm` when relevant.
5. External fallback is allowed only as a temporary safety net, not as the intended implementation path.

## Phase 0: Structure Refactor

Objective: stop `preprocessor.cpp` from growing as a single monolith.

Target outcomes:

1. split helper logic into focused modules
2. keep `Preprocessor` as the façade
3. preserve current behavior while improving change isolation

Work items:

1. extract text processing helpers
2. extract conditional state and helpers
3. extract include/path state and helpers
4. extract pragma handling entry points
5. extract predefined macro initialization
6. extract macro expansion helpers

Acceptance:

1. no intended behavior change
2. existing tests still pass
3. `preprocessor.cpp` becomes orchestration-focused instead of feature-dense

Recommended slice order:

1. text processing
2. include/path state
3. pragma dispatcher shell
4. predefined macros
5. macro expansion helpers
6. conditionals

## Phase 1: Output Contract and Public API

Objective: make the internal preprocessor a stable, configurable component.

Work items:

1. add public API for source-based preprocessing and file name control
2. add public API for define/undefine
3. split include paths into quote / normal / system / after buckets
4. add initial line marker emission
5. add include enter and include return markers
6. move `__FILE__`, `__LINE__`, `__BASE_FILE__`, and `__COUNTER__` into explicit managed state
7. define side-channel containers for pragma and macro-expansion metadata

Acceptance:

1. line markers are emitted deterministically without relying on external fallback
2. include boundaries are visible to downstream lexer/frontend stages
3. driver-side configuration is possible without patching internals directly

## Phase 2: Include System

Objective: remove include handling as the main reason for fallback.

Work items:

1. support `<...>` includes
2. support computed includes after macro expansion
3. implement full include search order
4. add include resolution cache
5. add `#include_next`
6. add `#pragma once`
7. add include guard optimization
8. preserve symlink-aware path behavior where practical

Acceptance:

1. `handle_include()` no longer falls back just because the include is not quoted
2. `__has_include` and `__has_include_next` can reuse the same resolver
3. include search behavior is testable and deterministic

## Phase 3: Pragma System

Objective: stop `#pragma` from forcing external fallback.

Work items:

1. implement a real pragma dispatcher
2. support `#pragma once`
3. support `#pragma pack(...)`
4. support `#pragma push_macro`
5. support `#pragma pop_macro`
6. support `#pragma weak`
7. support `#pragma redefine_extname`
8. support `#pragma GCC visibility push/pop`
9. define synthetic token or side-channel output format
10. make unknown pragmas ignore or warn deterministically

Acceptance:

1. `#pragma` no longer forces fallback by default
2. pragma side effects are visible through stable internal state or output

## Phase 4: `#if` Expressions and Intrinsics

Objective: make conditional compilation robust enough for real headers.

Work items:

1. separate expression preprocessing into explicit stages
2. implement real `__has_include` and `__has_include_next`
3. add builtin and attribute feature tables
4. improve literal parsing
5. replace fallback-on-parse-failure with deterministic internal handling

Expression preprocessing stages:

1. resolve `defined` and `__has_*`
2. expand macros
3. replace remaining identifiers with zero
4. evaluate integer constant expression

Acceptance:

1. common libc/compiler header conditions evaluate correctly
2. parse failures no longer silently escape into external fallback

## Phase 5: Macro Expansion Completeness

Objective: close the most important semantic gaps in macro behavior.

Work items:

1. support GNU named variadic macros
2. add anti-paste guard behavior
3. implement `_Pragma("...")`
4. introduce reusable expansion state
5. add macro expansion side-channel info
6. support multi-line function-like invocation accumulation

Acceptance:

1. macro behavior is closer to GCC/Clang on nontrivial test cases
2. multi-line macro invocation no longer depends on external preprocessing

## Phase 6: Predefined Macros and Target Configuration

Objective: stop assuming a single fixed host/target model.

Work items:

1. centralize predefined macro emission
2. add target-family macro sets for `x86_64`, `aarch64`, `riscv64`, `i686`, and `i386`
3. add config toggles for PIC, optimize, GNU89 inline, SIMD flags, and strict ANSI
4. add common compatibility macros such as `__GNUC__`, `__VERSION__`, `__STDC_HOSTED__`, `__DATE__`, and `__TIME__`

Acceptance:

1. common third-party headers stop selecting obviously wrong branches due to missing compatibility macros
2. target/config behavior is externally configurable

## Phase 7: Builtin Header Injection and Fallback Declarations

Objective: improve survivability when a full sysroot or system headers are unavailable.

Work items:

1. inject builtin macros for well-known headers
2. add builtin function-like macros such as `INT64_C(x)` and `UINT32_C(x)`
3. add minimal fallback declarations for a small set of common headers

Acceptance:

1. no-sysroot scenarios can pass more frontend cases
2. builtin substitutes are deterministic and explicitly scoped

## Testing Strategy

Every phase should add focused tests in the narrowest useful scope.

Core test groups:

1. macro expansion
2. conditionals
3. includes
4. pragmas
5. predefined macros
6. output contract and line markers

Validation modes:

1. targeted tests for the exact slice being implemented
2. golden-output comparisons for expanded text, markers, and diagnostics
3. Clang comparison harnesses for macro expansion, include resolution, and branch selection
4. full `ctest -j` regression check before handoff

## First Implementation Slices

Start here unless the codebase state clearly requires a smaller preparatory refactor.

1. split include path state into quote / normal / system / after buckets
2. add line marker infrastructure:
   - initial marker
   - include enter marker
   - include return marker
3. extract pragma handling into a dedicated function, even if the first supported pragma is only `#pragma once`

Rationale:

1. this reduces common fallback triggers quickly
2. this prepares the ground for `__has_include` and `#include_next`
3. this improves output stability without requiring a full macro-engine rewrite

## Plan Maintenance

1. `plan.md` is the authoritative feature roadmap for this track.
2. `plan_todo.md` is the iteration-to-iteration execution state derived from this file.
3. When the roadmap changes materially, update both `preprocessor_plan.md` and `plan.md`.
