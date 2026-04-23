---
name: c4cll-debug-flags
description: Use when working on c4cll frontend or backend debugging and you need to choose the right command-line flags for parser, sema, HIR, BIR, or codegen investigation. Covers parse-only, parser-debug channels, canonical dumps, HIR dumps, backend-route BIR observation, and practical command recipes for narrowing failures.
---

# c4cll Debug Flags

Use this skill when the task is to inspect compiler behavior through `c4cll`
flags instead of patching code first.

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

## Stage To Flag

- Parse / syntax:
  - `--parse-only`
- Parser control flow:
  - `--parser-debug`
  - `--parser-debug-tentative`
  - `--parser-debug-injected`
- Canonical semantic facts / type issues:
  - `--dump-canonical`
- HIR / template instantiation / compile-time behavior:
  - `--dump-hir-summary`
  - `--dump-hir`
- LIR / LLVM IR:
  - `--codegen llvm`
- Semantic BIR:
  - `--dump-bir`
- Prepared BIR:
  - `--dump-prepared-bir`
- MIR summary:
  - `--dump-mir`
- MIR trace:
  - `--trace-mir`
- Final backend asm:
  - `--codegen asm`

## Fast Triage

- "parse error / ambiguous syntax / backtracking":
  - start with `--parse-only`
  - add parser debug flags only as needed
- "wrong type / owner resolution / canonical form":
  - use `--dump-canonical`
- "template instantiation / compile-time execution / HIR lowering":
  - use `--dump-hir-summary`, then `--dump-hir`
- "LLVM path output is wrong":
  - use `--codegen llvm`
- "backend CFG / phi / legalization / regalloc prep is wrong":
  - use `--dump-bir`, then `--dump-prepared-bir`
  - for stack-specific reading, follow [debug_bir_stack.md](/workspaces/c4c/.codex/skills/c4cll-debug-flags/debug_bir_stack.md)
  - for frame-specific reading, follow [debug_bir_frame.md](/workspaces/c4c/.codex/skills/c4cll-debug-flags/debug_bir_frame.md)
  - for call-specific reading, follow [debug_bir_call.md](/workspaces/c4c/.codex/skills/c4cll-debug-flags/debug_bir_call.md)
- "machine lowering route is wrong":
  - use `--dump-mir`, then `--trace-mir`
- "final native output is wrong":
  - use `--codegen asm`
  - compare with `--codegen llvm` only to separate LLVM-path output from
    backend-native output

Rule of thumb:

1. Start with the first stage that can explain the symptom.
2. Do not jump to BIR/MIR until HIR looks structurally correct.
3. If `--codegen asm` fails but `--codegen llvm` looks healthy, treat it as a
   backend-native issue, not a frontend issue.

## Command Recipes

Frontend:

```bash
./build/c4cll --parse-only <file>
./build/c4cll --parser-debug --parse-only <file>
./build/c4cll --parser-debug --parser-debug-tentative --parse-only <file>
./build/c4cll --parser-debug --parser-debug-injected --parse-only <file>
./build/c4cll --dump-canonical <file>
./build/c4cll --dump-hir-summary <file>
./build/c4cll --dump-hir <file>
```

Midend and backend:

```bash
./build/c4cll --codegen llvm --target <triple> <file> -o /tmp/out.ll
./build/c4cll --dump-bir --target <triple> <file>
./build/c4cll --dump-prepared-bir --target <triple> <file>
./build/c4cll --dump-prepared-bir --target <triple> --mir-focus-function <function> <file>
./build/c4cll --dump-mir --target <triple> <file>
./build/c4cll --trace-mir --target <triple> <file>
./build/c4cll --codegen asm --target <triple> <file> -o /tmp/out.s
```

Compare LLVM path and backend-native path:

```bash
./build/c4cll --codegen asm --target <triple> <file> -o /tmp/backend.txt
./build/c4cll --codegen llvm --target <triple> <file> -o /tmp/llvm.txt
```

## Constraints

- `--pp-only`, `--lex-only`, `--parse-only`, `--dump-canonical`, `--dump-hir`,
  `--dump-hir-summary`, `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`,
  and `--trace-mir` are mutually exclusive inspection modes
- `--backend-bir-stage` is for `--codegen asm` only; do not combine it with
  `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, or `--trace-mir`
- `--dump-mir` and `--trace-mir` are currently route-level MIR visibility, not
  a full printed target-local MIR graph

## Prepared BIR Contract Sections

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

## Repo Examples

```bash
./build/c4cll --parse-only tests/cpp/internal/parse_only_case/top_level_empty_namespace_block_preserves_following_decl_parse.cpp
./build/c4cll --parser-debug --parser-debug-tentative --parse-only tests/cpp/internal/negative_case/parser_debug_if_condition_decl_tentative_lite.cpp
./build/c4cll --dump-hir-summary tests/cpp/internal/hir_case/hir_stmt_local_decl_helper_hir.cpp
./build/c4cll --dump-bir --target x86_64-unknown-linux-gnu tests/c/internal/positive_case/inline_param_bind.c
./build/c4cll --trace-mir --target x86_64-unknown-linux-gnu tests/c/internal/positive_case/inline_param_bind.c
```
