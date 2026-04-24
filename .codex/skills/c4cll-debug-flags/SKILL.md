---
name: c4cll-debug-flags
description: Use when working on c4cll frontend or backend debugging and you need to choose the right command-line flags for parser, sema, HIR, BIR, or codegen investigation. Covers parse-only, parser-debug channels, canonical dumps, HIR dumps, backend-route BIR observation, and practical command recipes for narrowing failures.
---

# c4cll Debug Flags

Use this skill when the task is to investigate frontend or backend behavior
through `c4cll` command-line flags instead of immediately patching the code.

## Pipeline Map

- `C/C++ -> HIR -> LIR -> BIR -> MIR -> assembly`
- `HIR`: frontend semantic stage; owns template instantiation and Sema-like work
- `LIR`: LLVM IR route; inspect with `--codegen llvm`
- `BIR`: backend IR for CFG, phi, legalization, register allocation, and preparation
- `MIR`: machine-lowering route after BIR
- `assembly`: final backend-native text from `--codegen asm`

Current limitation:

- there is no standalone `--dump-lir`
- use `--codegen llvm` for LLVM IR
- `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, and `--trace-mir` are
  backend-native evidence, not LLVM-path evidence

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
- LLVM path output is wrong:
  - use `--codegen llvm`

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
  - use `--dump-bir`
- Backend CFG / phi / legalization / regalloc prep is wrong:
  - use `--dump-bir`, then `--dump-prepared-bir`
  - for stack-specific reading, follow [debug_bir_stack.md](/workspaces/c4c/.codex/skills/c4cll-debug-flags/debug_bir_stack.md)
  - for frame-specific reading, follow [debug_bir_frame.md](/workspaces/c4c/.codex/skills/c4cll-debug-flags/debug_bir_frame.md)
  - for call-specific reading, follow [debug_bir_call.md](/workspaces/c4c/.codex/skills/c4cll-debug-flags/debug_bir_call.md)
- Machine lowering route is wrong:
  - use `--dump-mir`, then `--trace-mir`
- Final native output is wrong:
  - use `--codegen asm`
  - compare with `--codegen llvm` only to separate LLVM-path output from
    backend-native output

### Command recipes

Semantic BIR:

```bash
./build/c4cll --dump-bir --target <triple> <file>
```

Prepared BIR:

```bash
./build/c4cll --dump-prepared-bir --target <triple> <file>
./build/c4cll --dump-prepared-bir --target <triple> --mir-focus-function <function> <file>
```

MIR route:

```bash
./build/c4cll --dump-mir --target <triple> <file>
./build/c4cll --trace-mir --target <triple> <file>
```

Backend route versus LLVM route:

```bash
./build/c4cll --codegen asm --target <triple> <file> -o /tmp/backend.s
./build/c4cll --codegen llvm --target <triple> <file> -o /tmp/llvm.ll
```

Force semantic BIR through the asm route:

```bash
./build/c4cll --codegen asm --backend-bir-stage semantic --target <triple> <file>
```

### Investigation workflow

1. First ask whether the question is about semantic BIR, prepared BIR, MIR
   route selection, or final machine asm.
2. Use `--dump-bir` to verify semantic lowering first.
3. Use `--dump-prepared-bir` to verify backend-facing authority next.
4. Use `--dump-mir` or `--trace-mir` only after prepared BIR no longer answers
   the ownership or route question.
5. Use `--codegen llvm` only as the comparison surface, not as proof that the
   backend route is healthy.

### Prepared BIR contract sections

Current `--dump-prepared-bir` output is organized into ownership-oriented
sections:

- `--- prepared-control-flow ---`
- `--- prepared-value-locations ---`
- `--- prepared-stack-layout ---`
- `--- prepared-frame-plan ---`
- `--- prepared-dynamic-stack-plan ---`
- `--- prepared-call-plans ---`
- `--- prepared-storage-plans ---`
- `--- prepared-addressing ---`

Reading rule:

1. use `--dump-bir` to verify semantic lowering first
2. use `--dump-prepared-bir` to verify backend-facing authority next
3. do not treat `--dump-mir` or `--trace-mir` as the source of truth for
   stack, frame, or call ownership if the prepared dump already answers it

### Practical heuristics

- If the question is "did this testcase reach BIR yet?":
  - prefer `--dump-bir`
- If the question is "did this route stay on backend lowering or bounce back to
  LLVM-ish text?":
  - compare `--codegen asm` and `--codegen llvm`
- If the question is target-specific:
  - rerun the same testcase with the exact target triple you care about
- If the question is route drift:
  - keep one known-good semantic BIR testcase nearby as a sentinel

### Constraints

- `--pp-only`, `--lex-only`, `--parse-only`, `--dump-canonical`, `--dump-hir`,
  `--dump-hir-summary`, `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`,
  and `--trace-mir` are mutually exclusive inspection modes.
- `--backend-bir-stage` is for `--codegen asm` only; do not combine it with
  `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, or `--trace-mir`.
- `--dump-mir` and `--trace-mir` are route-level MIR visibility, not a full
  printed target-local MIR graph.

### Examples for current repo

Minimal BIR route check:

```bash
./build/c4cll --dump-bir --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/branch_if_eq.c
```

Prepared BIR route check:

```bash
./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/branch_if_eq.c
```

Backend route compare on the same testcase:

```bash
./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/c/internal/backend_case/call_helper.c -o /tmp/call_helper_backend.s
./build/c4cll --codegen llvm --target riscv64-unknown-linux-gnu tests/c/internal/backend_case/call_helper.c -o /tmp/call_helper_llvm.ll
```
