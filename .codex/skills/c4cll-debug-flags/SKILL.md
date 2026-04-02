---
name: c4cll-debug-flags
description: Use when working on c4cll frontend debugging and you need to choose the right command-line flags for parser, sema, HIR, or codegen investigation. Covers parse-only, parser-debug channels, canonical dumps, HIR dumps, and practical command recipes for narrowing failures.
---

# c4cll Debug Flags

Use this skill when the task is to investigate frontend behavior through `c4cll`
command-line flags instead of immediately patching the code.

## Quick selection

- Parser shape / syntax issue:
  - start with `--parse-only`
- Parser control-flow / backtracking issue:
  - add `--parser-debug`
  - add `--parser-debug-tentative` for speculative-parse visibility
  - add `--parser-debug-injected` when token injection/swap might be involved
- Sema / canonical type issue:
  - use `--dump-canonical`
- HIR lowering / compile-time behavior:
  - use `--dump-hir-summary`
  - escalate to `--dump-hir` when full detail is needed

## Command recipes

Basic parser check:

```bash
./build/c4cll --parse-only <file>
```

Parser debug:

```bash
./build/c4cll --parser-debug --parse-only <file>
```

Parser debug with speculative branches:

```bash
./build/c4cll --parser-debug --parser-debug-tentative --parse-only <file>
```

Parser debug with injected-token events:

```bash
./build/c4cll --parser-debug --parser-debug-injected --parse-only <file>
```

Full parser debug for hard parser cases:

```bash
./build/c4cll --parser-debug --parser-debug-tentative --parser-debug-injected --parse-only <file>
```

Canonical semantic view:

```bash
./build/c4cll --dump-canonical <file>
```

Compact HIR view:

```bash
./build/c4cll --dump-hir-summary <file>
```

Full HIR plus compile-time/materialization stats:

```bash
./build/c4cll --dump-hir <file>
```

## Investigation workflow

1. Start with the narrowest mode that answers the current question.
2. Prefer `--parse-only` before `--dump-hir` when the failure may still be in parser land.
3. If parser debug output is noisy, rerun with a reduced testcase before adding more channels.
4. If parse succeeds but later stages fail, switch to `--dump-canonical` or `--dump-hir-summary`.
5. Use full `--dump-hir` only when summary output is insufficient.

## Practical heuristics

- If the testcase hangs or runs very slowly during parse:
  - use `--parser-debug` first to see progress heartbeats
- If a failure involves nested templates or dependent names:
  - include `--parser-debug-tentative`
- If the code path is known to inject synthetic tokens:
  - include `--parser-debug-injected`
- If a bug report is about "wrong type" rather than "won't parse":
  - prefer `--dump-canonical`
- If a bug report is about template lowering, deferred consteval, or NTTP materialization:
  - prefer `--dump-hir-summary`, then `--dump-hir`

## Constraints

- `--pp-only`, `--lex-only`, `--parse-only`, `--dump-canonical`, `--dump-hir`,
  and `--dump-hir-summary` are mutually exclusive frontend inspection modes.
- Parser debug flags are most useful with `--parse-only`, but can also help when
  earlier frontend stages need observation before later failures.

## Examples for current repo

STL parser bring-up:

```bash
./build/c4cll --parser-debug --parser-debug-tentative --parse-only tests/cpp/std/std_vector_simple.cpp
```

EASTL parser bring-up:

```bash
./build/c4cll --parser-debug --parser-debug-tentative --parser-debug-injected --parse-only tests/cpp/eastl/eastl_vector_simple.cpp
```

Known parser-debug negative testcase:

```bash
./build/c4cll --parser-debug --parser-debug-tentative --parse-only tests/cpp/internal/negative_case/parser_debug_qualified_type_template_arg_stack.cpp
```
