# Preprocessor Subsystem

The preprocessor is a text-to-text frontend pass. It reads raw source text or a
source file, applies directive handling and macro expansion, and emits expanded
source text annotated with GCC-style line markers for the lexer.

In this codebase, the preprocessor is not a thin wrapper around an external
tool. It owns include resolution, conditional compilation, macro expansion,
pragma preservation, and several built-in macro behaviors directly in C++.

---

## Table of Contents

1. [Module Organization](#module-organization)
2. [Public Interface](#public-interface)
3. [Overall Pipeline](#overall-pipeline)
4. [Macro Model](#macro-model)
5. [Conditional Compilation](#conditional-compilation)
6. [Include Resolution](#include-resolution)
7. [Line Markers and Virtual Locations](#line-markers-and-virtual-locations)
8. [Pragmas](#pragmas)
9. [Diagnostics and Fallback Signals](#diagnostics-and-fallback-signals)
10. [Invariants](#invariants)

---

## Module Organization

```txt
preprocessor/
  preprocessor.hpp    -- Preprocessor class, diagnostics, public API
  preprocessor.cpp    -- main orchestration and directive loop
  pp_text.*           -- line splicing, comment stripping, text helpers
  pp_cond.*           -- #if / #ifdef / #elif expression handling
  pp_include.*        -- include lookup and file loading helpers
  pp_macro_def.hpp    -- MacroDef and macro-table data structures
  pp_macro_expand.*   -- macro expansion, substitution, stringify/paste helpers
  pp_pragma.*         -- pragma dispatch helpers
  pp_predefined.*     -- predefined macro initialization
```

The subsystem is intentionally split by responsibility: raw text cleanup,
conditional logic, include handling, macro expansion, and predefined-macro
setup all live in separate helper units, while `preprocessor.cpp` owns the
top-level pass.

## Public Interface

The main public surface lives in `preprocessor.hpp`:

- `Preprocessor()`
- `preprocess_file(path)`
- `preprocess_source(source, filename)`
- include path configuration via `-I` / `-iquote` / `-isystem` / `-idirafter`
- driver-style macro control through `define_macro(...)` / `undefine_macro(...)`
- source-profile selection through `set_source_profile(...)`
- side-channel diagnostics through `errors()` and `warnings()`

This is the API used by `c4cll.cpp` before the lexer runs.

## Overall Pipeline

At a high level the preprocessor does:

1. initialize/reset state for a new preprocessing run
2. inject predefined macros such as `__BASE_FILE__`
3. emit an initial line marker for the root source
4. preprocess the active file text through `preprocess_text(...)`

Inside `preprocess_text(...)`, the pass looks like:

1. join continued lines
2. strip comments
3. iterate line-by-line
4. update virtual `__LINE__` / `__FILE__`
5. if the line is a directive, route it to `process_directive(...)`
6. otherwise, if the current conditional state is active, expand macros
7. otherwise emit a blank line to preserve line numbering

The output is still text, not tokens.

## Macro Model

Macros are stored in `MacroTable` as `MacroDef` records.
The implementation supports:

- object-like macros
- function-like macros
- variadic macros
- GNU named-variadic forms
- recursive-expansion suppression through a disabled set
- stringification and token pasting
- command-line `-D` / `-U` style driver mutations

Expansion happens on active non-directive lines via `expand_line(...)` and
`expand_text(...)`.

The preprocessor also accumulates multiline function-like invocations when a
line has more opening than closing parentheses, so a macro call can span
multiple physical lines without being truncated mid-argument list.

## Conditional Compilation

Conditional state is tracked by `ConditionalFrame`:

- `parent_active`
- `any_taken`
- `this_active`
- `saw_else`

This is enough to model nested `#if` / `#ifdef` / `#ifndef` / `#elif` /
`#else` / `#endif` blocks while preserving the distinction between:

- "the parent region is inactive"
- "this branch was skipped because an earlier branch already matched"
- "this branch is the active one"

`evaluate_if_expr(...)` handles `#if` and `#elif` expressions after resolving
macro-definedness and include-related intrinsics.

Inactive branches still emit blank lines so that downstream line/column mapping
remains stable.

## Include Resolution

The preprocessor owns include lookup itself.

Search buckets mirror GCC-style behavior:

- current file directory
- `quote_include_paths_` for `-iquote`
- `normal_include_paths_` for `-I`
- `system_include_paths_` for `-isystem`
- `after_include_paths_` for `-idirafter`

Important supporting features include:

- `#include` and `#include_next`
- include-depth limiting
- `#pragma once`
- simple include-guard detection / reinclude skipping
- builtin-header injection for selected well-known system headers
- include-resolution caching

The source profile also influences include policy, for example whether
certain header suffixes are acceptable in the current mode.

## Line Markers and Virtual Locations

The preprocessor emits GCC-style line markers such as:

```c
# 1 "path/to/file.c"
```

It also tracks virtual location state for `#line`:

- `virtual_line_offset_`
- `virtual_file_`

These fields keep `__LINE__`, `__FILE__`, and downstream diagnostics aligned
with the user-visible source location rather than just the physical file buffer.

This is why the lexer can later skip line-marker lines while still reporting
useful locations.

## Pragmas

Pragmas are split into three categories:

1. pragmas handled entirely inside the preprocessor
2. pragmas preserved into output text for the lexer/parser pipeline
3. unhandled pragmas that mark the source as needing external fallback

The currently preserved parser-facing forms are:

- `#pragma pack(...)`
- `#pragma weak ...`
- `#pragma GCC visibility ...`

These are written back into the expanded output so the lexer can turn them into
synthetic tokens.

The preprocessor also supports `#pragma once` and push/pop of macro definitions
through `push_macro` / `pop_macro`.

## Diagnostics and Fallback Signals

Diagnostics are side-channel data, not inline text insertions:

- `errors_` collects failures such as invalid directives or missing includes
- `warnings_` collects `#warning` and similar non-fatal cases

There is also a coarse fallback signal:

- `needs_external_fallback_`

This flag is set when the preprocessor encounters directive shapes or pragma
forms it does not confidently implement yet. That allows higher-level tooling
to notice that preprocessing stayed on a best-effort path.

## Invariants

- The preprocessor consumes and emits text, not tokens.
- Line count preservation matters: inactive branches and many directive paths
  still emit blank lines intentionally.
- Include-path search order must remain GCC-compatible.
- Parser-facing pragmas are preserved as text so the lexer can synthesize typed
  tokens from them later.
- Per-run state such as conditional stacks, pragma-once data, include caches,
  and diagnostics must be reset at the start of each preprocessing entry point.
