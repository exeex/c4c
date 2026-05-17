# Backend and AArch64 Codegen Entrypoint Clarity

Status: Open
Created: 2026-05-17

## Intent

Make the backend driver and AArch64 codegen route obvious to humans and agents.

`src/backend/backend.cpp` should read like a high-level backend flow, not like
the owner of AArch64 MIR printing details. `src/backend/mir/aarch64/codegen/`
should expose one clear public compile entry and keep its internal lowering
entry points named by responsibility.

This idea follows the public interface cleanup in
`ideas/open/263_aarch64_codegen_public_compiled_module_interface.md`. It should
not fight 263; it should build on the existence of `aarch64::codegen` as the
public compiled-module boundary.

## Why This Exists

The current route is hard to read:

- `backend.cpp` contains generic target routing, BIR preparation, route-debug
  helpers, and AArch64 machine-node-to-assembly rendering in the same file.
- The AArch64 codegen entry appears to be `codegen/emit.cpp`, but that file is
  really the prepared-module compile coordinator, not an assembly emitter.
- The deeper route from prepared BIR to MIR nodes is split through traversal
  and dispatch, but the names do not make the call graph obvious from the
  public entry.
- Agent review becomes expensive because it must reconstruct whether a change
  belongs to the backend driver, the AArch64 compile pipeline, the MIR printer,
  or a future assembler handoff.

The desired reading order is:

1. backend driver resolves route and input stage;
2. AArch64 public codegen compiles prepared BIR into a compiled module;
3. a printer or assembler consumer walks that compiled module;
4. lowering internals stay in codegen files named after their job.

## In Scope

- Make `src/backend/backend.cpp` a thin flow coordinator:
  target selection, LIR/BIR/prepared handoff, route-debug selection, and calls
  to target-owned public APIs.
- Move AArch64-specific machine-node assembly rendering out of `backend.cpp`
  into an AArch64-owned helper under `src/backend/mir/aarch64/`.
- Give AArch64 codegen a clear public entry, expected to come from 263:
  `src/backend/mir/aarch64/codegen/codegen.hpp` and a function such as
  `codegen::compile_prepared_module(...)`.
- Rename or split misleading internal entry files where behavior-preserving:
  `emit.cpp` should not be the only discoverable "entry" if it is really the
  prepared-module compile coordinator.
- Make the internal AArch64 flow easy to trace from names alone:
  public compile entry -> module compile coordinator -> function/block
  traversal -> instruction dispatch -> family lowerers.
- Keep the output consumer boundary explicit:
  shared MIR printer may consume the compiled module for `.s` text today, and a
  future in-process assembler can consume the same compiled module later.
- Update docs, tests, or contract text that describe the route so review agents
  do not have to infer the architecture from scattered implementation details.

## Out of Scope

- Changing lowering semantics, instruction selection, diagnostics, or emitted
  assembly.
- Replacing the shared MIR container or target printer boundary; that belongs
  to the common MIR/printer work.
- Implementing a native assembler or object writer in this idea.
- Moving every AArch64 internal helper out of `module.hpp` merely for namespace
  aesthetics.
- Broad file shuffling that makes the call graph look different without
  reducing `backend.cpp` responsibility or clarifying the public codegen entry.

## Proposed Shape

The desired top-level driver shape is:

```text
backend.cpp
  resolve target/profile/input route
  prepare semantic BIR when needed
  call target-owned public route
    AArch64:
      codegen::compile_prepared_module(prepared)
      aarch64 printer/assembly helper renders compiled module when .s is requested
```

The desired AArch64 codegen shape is:

```text
src/backend/mir/aarch64/codegen/codegen.hpp
  public CompiledModule / CompileResult / compile_prepared_module

src/backend/mir/aarch64/codegen/<compile entry>.cpp
  validate prepared-module handoff
  create compiled module shell
  lower module body
  return CompileResult

src/backend/mir/aarch64/codegen/traversal.cpp
  lower functions and blocks from prepared BIR

src/backend/mir/aarch64/codegen/dispatch.cpp
  dispatch prepared operations to family lowerers

src/backend/mir/aarch64/codegen/{alu,memory,calls,...}.cpp
  construct target instructions for each operation family
```

The desired text assembly output shape is:

```text
src/backend/mir/aarch64/<printer-or-assembly helper>
  take codegen::CompiledModule
  use shared mir::print_machine_function plus AArch64 target instruction printer
  produce .s text for the current external assembler route
```

The exact filenames can follow local naming during implementation, but the
responsibility boundary should remain visible.

## Suggested Execution Order

1. Finish or land 263 first, so `aarch64::codegen` owns the public compiled
   module API.
2. Audit `backend.cpp` and classify each helper as one of:
   generic driver, x86 route, AArch64 route, route-debug filtering, or shared
   BIR preparation.
3. Extract AArch64 `.s` text rendering from `backend.cpp` into a target-owned
   helper that takes the compiled module or prepared module through the public
   codegen API.
4. Rename or wrap the AArch64 compile coordinator so the public entry and the
   internal compile coordinator are both discoverable without reading the full
   call graph.
5. Update route docs/tests/contracts to describe:
   prepared BIR -> compiled MIR module -> shared MIR printer or future
   assembler consumer.
6. Validate that assembly output and public backend smoke tests are unchanged.

## Completion Criteria

- `backend.cpp` no longer owns AArch64 machine-node assembly rendering details.
- A reviewer can identify the backend flow from `backend.cpp` without opening
  AArch64 lowering internals.
- A reviewer can identify the AArch64 public codegen entry from
  `codegen/codegen.hpp`.
- A reviewer can identify the internal prepared-BIR-to-MIR route from file and
  function names without reconstructing it from unrelated helpers.
- The current `.s` route still uses the shared MIR printer and target-specific
  instruction/operand printing.
- The same compiled module structure remains suitable for a future in-process
  assembler or object writer.
- Focused AArch64 backend/MIR proof and public assembly smoke remain green, or
  any host-tool skips are explicitly reported.

## Reviewer Reject Signals

Reject the route or slice if it:

- keeps AArch64 machine-node assembly rendering in `backend.cpp` while claiming
  the driver was clarified;
- renames files or functions without reducing the review burden of finding the
  backend driver entry and AArch64 codegen entry;
- routes public callers around `aarch64::codegen` after 263 instead of through
  the codegen-owned compiled-module API;
- changes lowering semantics, emitted assembly, diagnostics, or test
  expectations while claiming the work is behavior-preserving;
- creates testcase-shaped shortcuts or named-case-only paths to keep a smoke
  test passing;
- hides the future assembler boundary by making `.s` text strings the only
  reusable product of AArch64 codegen;
- broadens into a common MIR container/printer redesign without an explicit
  switch to the relevant common-MIR idea.
