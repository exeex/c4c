# LIR HIR-To-LIR Index Tightening

## Intent

Tighten the `src/codegen/lir` header layout after the first LIR hierarchy
cleanup so headers act as useful LLM-agent indexes instead of scattered lookup
targets.

The goal is to make the top-level LIR headers read like exported package
surfaces, while nested HIR-to-LIR headers read like private implementation
indexes for the semantic area an agent has already entered.

## Rationale

The first LIR agent-index pass moved HIR-to-LIR implementation files under
`src/codegen/lir/hir_to_lir/`, created private `call/` and `expr/` subdomains,
and folded thin printer/verify declarations into `ir.hpp`.

The remaining question is whether the current header shape gives agents the
fastest route to the right context:

```text
src/codegen/lir/*.hpp
  exported LIR package surfaces for src/** callers

src/codegen/lir/hir_to_lir/*.hpp
  private index for the HIR-to-LIR lowering implementation

src/codegen/lir/hir_to_lir/call/*.hpp
  private index for call-lowering internals

src/codegen/lir/hir_to_lir/expr/*.hpp
  private index for expression-lowering internals
```

For LLM agents, each extra candidate header costs search and read budget.
Headers should therefore answer "what can I use from this boundary?" quickly,
not mirror each implementation file.

## Current Shape

Notable current headers:

- `src/codegen/lir/ir.hpp`
  - public LIR IR model plus small LIR-level printer and verifier APIs
- `src/codegen/lir/hir_to_lir.hpp`
  - public HIR-to-LIR entry surface
- `src/codegen/lir/call_args.hpp`
  - exported call argument data model used outside HIR-to-LIR lowering
- `src/codegen/lir/call_args_ops.hpp`
  - exported call argument operations used outside HIR-to-LIR lowering
- `src/codegen/lir/operands.hpp`
  - top-level LIR operand model included by `ir.hpp`
- `src/codegen/lir/types.hpp`
  - top-level LIR type model included by `ir.hpp`
- `src/codegen/lir/hir_to_lir/lowering.hpp`
  - private HIR-to-LIR implementation index
- `src/codegen/lir/hir_to_lir/call/call.hpp`
  - private call-lowering index
- `src/codegen/lir/hir_to_lir/expr/expr.hpp`
  - private expression-lowering index

Current implementation directories:

```text
src/codegen/lir/
  lir_printer.cpp
  verify.cpp

src/codegen/lir/hir_to_lir/
  hir_to_lir.cpp
  core.cpp
  const_init_emitter.cpp
  lvalue.cpp
  stmt.cpp
  types.cpp

src/codegen/lir/hir_to_lir/call/
  args.cpp
  builtin.cpp
  coordinator.cpp
  target.cpp
  vaarg.cpp
  vaarg_amd64.cpp
  vaarg_amd64_registers.cpp

src/codegen/lir/hir_to_lir/expr/
  coordinator.cpp
  binary.cpp
  misc.cpp
```

## Design Direction

Use this rule:

```text
directory = semantic boundary
.hpp      = index entry for that boundary
```

Top-level headers should be exported surfaces only. Nested headers should be
private implementation indexes. Do not add a matching `.hpp` for each `.cpp`.

When a top-level header is only an implementation convenience, move or fold it
behind the relevant nested index. When a nested header is only a forwarder with
little indexing value, strengthen it or merge it into the nearest useful
private index.

## Refactoring Steps

1. Reclassify top-level LIR headers.
   - Keep `ir.hpp` as the public LIR package index for IR, printer, and
     verifier surfaces.
   - Keep `hir_to_lir.hpp` as the public HIR-to-LIR entry surface.
   - Confirm whether `call_args.hpp` and `call_args_ops.hpp` are intentionally
     exported because BIR lowering, printer, or other codegen users need them.
   - Decide whether `operands.hpp` and `types.hpp` should remain visible
     top-level model headers or be treated as implementation details of
     `ir.hpp`.

2. Tighten `hir_to_lir.hpp` as the public entry.
   - Ensure external callers can understand and use HIR-to-LIR lowering from
     this header without opening private `hir_to_lir/lowering.hpp`.
   - Do not expose statement-emitter, call-emitter, expression-emitter, or
     lowering helper internals through this top-level header.

3. Strengthen `hir_to_lir/lowering.hpp` as the private lowering index.
   - Ensure agents editing core HIR-to-LIR lowering can open this one file to
     find the shared lowering context, common helper declarations, and
     subdomain entry points.
   - Keep implementation-only helpers behind this private boundary.
   - Avoid turning it into a dumping ground for call/expr details that belong
     in subdomain indexes.

4. Strengthen `hir_to_lir/call/call.hpp`.
   - Make it the single private index for call lowering internals.
   - It should expose the call-lowering helper declarations needed by
     `call/*.cpp` and the parent lowering coordinator.
   - Do not create `args.hpp`, `builtin.hpp`, `target.hpp`, or per-ABI headers
     unless a real nested semantic directory is introduced later.

5. Strengthen `hir_to_lir/expr/expr.hpp`.
   - Make it the single private index for expression lowering internals.
   - It should expose expression-lowering helper declarations needed by
     `expr/*.cpp` and the parent lowering coordinator.
   - Do not create `binary.hpp`, `misc.hpp`, or per-expression headers.

6. Preserve behavior.
   - This initiative is structural and navigational.
   - It should not change LIR semantics, lowering behavior, printer output,
     verifier behavior, or testcase expectations.
   - Semantic cleanup discovered during this work should become a separate
     idea unless required to keep the structural refactor compiling.

## Acceptance Criteria

- Top-level `src/codegen/lir/*.hpp` files are either clear exported surfaces or
  deliberately documented as model subheaders used by the public LIR index.
- External HIR-to-LIR users can treat `src/codegen/lir/hir_to_lir.hpp` as the
  only public entry header.
- HIR-to-LIR implementation agents can treat
  `src/codegen/lir/hir_to_lir/lowering.hpp` as the private implementation
  index.
- Call-lowering agents can treat
  `src/codegen/lir/hir_to_lir/call/call.hpp` as the single private call index.
- Expression-lowering agents can treat
  `src/codegen/lir/hir_to_lir/expr/expr.hpp` as the single private expression
  index.
- No one-header-per-`.cpp` pattern is introduced.
- `c4c_codegen` builds after each structural slice.
- Relevant frontend/HIR-to-LIR tests continue to pass.

## Non-Goals

- Do not introduce a traditional separated `include/` tree.
- Do not use headers primarily to enforce C++ interface purity.
- Do not split every `.cpp` into a matching `.hpp`.
- Do not move large implementation families again unless the header-index
  review proves the current semantic directory is wrong.
- Do not change HIR-to-LIR semantics as part of this layout work.
