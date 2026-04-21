---
name: c4cll-debug-flags
description: Use when working on c4cll frontend or backend debugging and you need to choose the right command-line flags for parser, sema, HIR, BIR, or codegen investigation. Covers parse-only, parser-debug channels, canonical dumps, HIR dumps, backend-route BIR observation, and practical command recipes for narrowing failures.
---

# c4cll Debug Flags

Use this skill when the task is to investigate frontend or backend behavior
through `c4cll` command-line flags instead of immediately patching the code.

## Frontend Debug

### Quick selection

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

### Command recipes

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

### Investigation workflow

1. Start with the narrowest mode that answers the current question.
2. Prefer `--parse-only` before `--dump-hir` when the failure may still be in parser land.
3. If parser debug output is noisy, rerun with a reduced testcase before adding more channels.
4. If parse succeeds but later stages fail, switch to `--dump-canonical` or `--dump-hir-summary`.
5. Use full `--dump-hir` only when summary output is insufficient.

### Practical heuristics

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

### Constraints

- `--pp-only`, `--lex-only`, `--parse-only`, `--dump-canonical`, `--dump-hir`,
  and `--dump-hir-summary` are mutually exclusive frontend inspection modes.
- Parser debug flags are most useful with `--parse-only`, but can also help when
  earlier frontend stages need observation before later failures.

### Examples for current repo

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

## Backend Debug

### Quick selection

- Semantic backend route facts:
  - use `--dump-bir`
- Prepared backend facts plus control-flow / home / addressing metadata:
  - use `--dump-prepared-bir`
- Concise x86 MIR-route status:
  - use `--dump-mir`
- Verbose x86 MIR-route trace:
  - use `--trace-mir`
- Backend route comparison against LLVM path:
  - compare `--codegen asm` and `--codegen llvm` on the same testcase

### Command recipes

Semantic BIR observation:

```bash
./build/c4cll --dump-bir --target <triple> <file>
```

Prepared BIR observation:

```bash
./build/c4cll --dump-prepared-bir --target <triple> <file>
```

Concise MIR-route summary:

```bash
./build/c4cll --dump-mir --target <triple> <file>
```

Verbose MIR-route trace:

```bash
./build/c4cll --trace-mir --target <triple> <file>
```

Backend route versus LLVM route:

```bash
./build/c4cll --codegen asm --target <triple> <file> -o /tmp/backend.txt
./build/c4cll --codegen llvm --target <triple> <file> -o /tmp/llvm.txt
```

### Investigation workflow

1. First ask which backend layer you are trying to observe.
2. Use `--dump-bir` for semantic BIR only.
3. Use `--dump-prepared-bir` when you need prepared control-flow, value-home,
   move-bundle, stack-layout, or addressing metadata.
4. Use `--dump-mir` when you want a short answer about which top-level x86
   route lanes matched or failed.
5. Use `--trace-mir` when you need the same information as a lane-by-lane
   trace, including backend rejection detail when a lane throws or declines.
6. Use `--codegen llvm` only as the comparison surface, not as proof that the
   backend route is healthy.

### Practical heuristics

- If the question is "did this testcase reach semantic BIR yet?":
  - prefer `--dump-bir`
- If the question is "what prepared facts exist for this route?":
  - prefer `--dump-prepared-bir`
- If the question is "which x86 lane is this function trying and failing?":
  - start with `--dump-mir`, then escalate to `--trace-mir`
- If the question is target-specific:
  - rerun the same testcase with the exact target triple you care about
- If the question is route drift:
  - keep one known-good semantic BIR testcase nearby as a sentinel

### Constraints

- `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, and `--trace-mir` are
  mutually exclusive inspection modes.
- `--backend-bir-stage` is for `--codegen asm` only. Do not combine it with
  `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, or `--trace-mir`.
- Current `--dump-mir` / `--trace-mir` are a backend MIR-route shell for x86
  prepared handoff, not yet a full printed target-local MIR node graph.
- Use `--codegen asm` when you want final backend-native asm output, not when
  you want structured intermediate debug text.

### Examples for current repo

Minimal semantic BIR route check:

```bash
./build/c4cll --dump-bir --target x86_64-unknown-linux-gnu tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c
```

Prepared route check:

```bash
./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c
```

Concise MIR-route check:

```bash
./build/c4cll --dump-mir --target x86_64-unknown-linux-gnu tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c
```

Verbose MIR trace on the same testcase:

```bash
./build/c4cll --trace-mir --target x86_64-unknown-linux-gnu tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c
```

Backend route compare on the same testcase:

```bash
./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/c/internal/backend_case/call_helper.c -o /tmp/call_helper_backend.txt
./build/c4cll --codegen llvm --target riscv64-unknown-linux-gnu tests/c/internal/backend_case/call_helper.c -o /tmp/call_helper_llvm.txt
```
