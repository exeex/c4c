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

- Backend route / semantic BIR investigation:
  - use `--codegen asm --target <triple> <file> -o <out>`
  - inspect `<out>` to see whether the backend emitted native asm, semantic
    BIR text, or LLVM-ish fallback text
- Backend route comparison against LLVM path:
  - compare `--codegen asm` and `--codegen llvm` on the same testcase

### Command recipes

Backend route / BIR observation:

```bash
./build/c4cll --codegen asm --target <triple> <file> -o <out>
cat <out>
```

Backend route versus LLVM route:

```bash
./build/c4cll --codegen asm --target <triple> <file> -o /tmp/backend.txt
./build/c4cll --codegen llvm --target <triple> <file> -o /tmp/llvm.txt
```

### Investigation workflow

1. First ask whether the question is about lowering route or final machine asm.
2. Use `--codegen asm --target <triple> ... -o <out>` to observe the backend
   route surface.
3. If `<out>` starts with `bir.func`, semantic BIR lowering succeeded and the
   current route printed BIR text.
4. If `<out>` looks like LLVM IR such as `define ...`, the route fell back to
   LIR/LLVM-style text instead of staying on the semantic BIR lane.
5. If `--codegen asm` errors out saying backend-native assembly is required,
   the chosen route did not produce native asm and did not return printable BIR
   text for that path.
6. Use `--codegen llvm` only as the comparison surface, not as proof that the
   backend route is healthy.

### Practical heuristics

- If the question is "did this testcase reach BIR yet?":
  - prefer `--codegen asm --target <triple> <file> -o <out>`
- If the question is "did this route stay on backend lowering or bounce back to
  LLVM-ish text?":
  - inspect whether the output starts with `bir.func` or LLVM-like `define`
- If the question is target-specific:
  - rerun the same testcase with the exact target triple you care about
- If the question is route drift:
  - keep one known-good semantic BIR testcase nearby as a sentinel

### Constraints

- There is currently no dedicated `--dump-bir` flag.
- Backend BIR observation currently piggybacks on `--codegen asm`.
- This route may print native asm, semantic BIR text, or LLVM-ish fallback
  text depending on how far the backend pipeline got for the chosen testcase
  and target.

### Examples for current repo

Minimal BIR route check:

```bash
./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/branch_if_eq.c -o /tmp/branch_if_eq.txt
cat /tmp/branch_if_eq.txt
```

Backend route compare on the same testcase:

```bash
./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/c/internal/backend_case/call_helper.c -o /tmp/call_helper_backend.txt
./build/c4cll --codegen llvm --target riscv64-unknown-linux-gnu tests/c/internal/backend_case/call_helper.c -o /tmp/call_helper_llvm.txt
```
