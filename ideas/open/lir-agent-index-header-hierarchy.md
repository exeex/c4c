# LIR Agent Index Header Hierarchy

## Intent

Apply the same LLM-agent-oriented header hierarchy strategy to
`src/codegen/lir` that is being introduced for `src/backend/bir`.

The goal is to make LIR easier for agents to navigate by using headers as
directory-level index surfaces, not as automatic one-to-one partners for
implementation files. Path depth should communicate visibility and ownership
with minimal token cost.

## Rationale

`src/codegen/lir` currently mixes public LIR IR surfaces, HIR-to-LIR entry
points, printer/verify helpers, call argument lowering, constant initializer
emission, and statement-emitter implementation state in one directory.

For an LLM agent, many small headers are not free. Each candidate header can
cost a file search, a file read, and additional context before the agent knows
whether it is useful. Traditional C++ header discipline is less important here
than making the directory tree itself a readable map.

Use path depth as the primary visibility signal:

```text
src/codegen/lir/*.hpp
  public or exported LIR surfaces for src/** callers

src/codegen/lir/hir_to_lir/*.hpp
  private to HIR-to-LIR lowering implementation

src/codegen/lir/hir_to_lir/call/*.hpp
  private to a call-lowering subdomain, if that subdomain is split out
```

Nested headers are private by default because agents only reach them after
entering that semantic area. Top-level headers are public by default because
they are visible at the package boundary. If a wider interface must be
exported, it should have an explicit top-level `.hpp`.

## Design Direction

Use this rule:

```text
directory = semantic boundary
.hpp      = index entry for that boundary
```

Avoid one header per `.cpp`. Prefer one meaningful header per semantic
directory. Thin headers should be merged into a nearby public or private index
unless they are explicit exported surfaces.

## Current Hotspots

Current notable files:

- `src/codegen/lir/ir.hpp`
  - public LIR IR model, currently includes `operands.hpp` and `types.hpp`
- `src/codegen/lir/hir_to_lir.hpp`
  - public HIR-to-LIR entry surface
- `src/codegen/lir/stmt_emitter.hpp`
  - private implementation index for statement/expression emission, but it
    currently lives at the public top level
- `src/codegen/lir/call_args.hpp`
  - large call argument model/helper surface
- `src/codegen/lir/call_args_ops.hpp`
  - call argument operations used by lowering/printer/BIR
- `src/codegen/lir/const_init_emitter.hpp`
  - constant initializer emission helper surface
- `src/codegen/lir/lir_printer.hpp`
  - thin printer declaration header
- `src/codegen/lir/verify.hpp`
  - verifier helper surface

## Target Shape

Preferred long-term layout:

```text
src/codegen/lir/
  ir.hpp
  ir.cpp                  # only if needed later
  hir_to_lir.hpp
  lir_printer.cpp
  verify.cpp

  hir_to_lir/
    lowering.hpp
    lowering.cpp          # former hir_to_lir.cpp, if useful as entry impl
    const_init.cpp
    stmt.cpp
    expr.cpp
    lvalue.cpp
    types.cpp

    call/
      call.hpp            # only if call lowering needs a subdomain index
      coordinator.cpp
      args.cpp
      builtin.cpp
      target.cpp
      vaarg.cpp
      vaarg_amd64.cpp
      vaarg_amd64_registers.cpp
```

The exact filenames can change during planning, but the hierarchy should keep
the public/private signal simple:

- top-level `*.hpp` files are exported surfaces
- `hir_to_lir/lowering.hpp` is the private index for HIR-to-LIR implementation
- `hir_to_lir/call/call.hpp` exists only if call lowering becomes its own
  semantic directory

## Refactoring Steps

1. Decide the top-level exported LIR surfaces.
   - Keep `ir.hpp` public for the LIR data model.
   - Keep `hir_to_lir.hpp` public for HIR-to-LIR entry APIs.
   - Decide whether `lir_printer.hpp` and `verify.hpp` should remain exported
     headers or merge into `ir.hpp` as small LIR-level APIs.
   - Do not leave implementation-only state in top-level headers.

2. Create a private HIR-to-LIR implementation directory index.
   - Add `src/codegen/lir/hir_to_lir/lowering.hpp`.
   - Move `StmtEmitter` and related implementation-only declarations out of
     top-level `stmt_emitter.hpp`.
   - Update `stmt_emitter_*.cpp`, `const_init_emitter.cpp`, and
     `hir_to_lir.cpp`-equivalent implementation files to include the private
     directory index.

3. Move implementation `.cpp` files under the matching semantic directory.
   - `stmt_emitter_*.cpp` should live under `hir_to_lir/`.
   - Call-heavy statement-emitter files may move under `hir_to_lir/call/` if
     that materially improves navigation.
   - Avoid creating one-off headers while doing this move.

4. Reassess call argument surfaces separately.
   - `call_args.hpp` and `call_args_ops.hpp` are used outside LIR lowering,
     including printer and BIR lowering.
   - If they are exported cross-component interfaces, keep them top-level.
   - If part of them is implementation-only, move only that private portion
     behind the `hir_to_lir/` hierarchy.

5. Preserve behavior.
   - This initiative is structural. It should not change LIR semantics,
     printing, verifier behavior, or testcase expectations.
   - Semantic cleanups discovered during the refactor should become separate
     ideas unless required to keep the structural move compiling.

## Acceptance Criteria

- External callers can include top-level LIR headers without pulling in
  statement-emitter implementation state.
- Agents editing HIR-to-LIR lowering can open one private index header,
  `src/codegen/lir/hir_to_lir/lowering.hpp`, to understand shared lowering
  declarations.
- Thin headers are reduced or justified as exported surfaces.
- No one-header-per-`.cpp` pattern is introduced.
- `c4c_codegen` builds after each structural slice.
- Relevant frontend/codegen tests continue to pass.

## Non-Goals

- Do not introduce a traditional separated `include/` tree.
- Do not use headers primarily to enforce C++ interface purity.
- Do not split every `.cpp` into a matching `.hpp`.
- Do not change HIR-to-LIR lowering semantics as part of this layout work.
