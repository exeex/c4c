# HIR -> LIR Refactor Plan

## Archive Status

Archived on 2026-03-26.

This refactor track is complete and closed.

Current end state:

- `src/codegen/lir/` owns lowering plus LLVM IR printing entry support.
- `src/codegen/llvm/llvm_codegen.cpp` is now a thin orchestration layer that routes codegen through the LIR pipeline.
- structured LIR replaced the old raw LLVM-text fallback nodes used during the transition.
- dead internal function elimination moved out of the printer; `lir_printer` now renders the LIR it receives.
- `StmtEmitter` and constant-initializer lowering now live under `src/codegen/lir/`.
- `src/codegen/shared/llvm_helpers.hpp` holds the remaining LLVM spelling helpers shared by lowering and printing.

Historical note:

- the temporary `legacy|lir|compare` split used during migration has been retired after validation; the repository now keeps the LIR path as the only active codegen path.

Archive note:

- detailed migration runbooks previously tracked in `plan.md` and `todo.md` have been folded back into this archived design document and removed.

## Goal

Split current `src/codegen/llvm/hir_emitter.cpp` into two layers:

1. `hir_to_lir`
   Transform HIR into a backend-oriented low-level IR while preserving the existing frontend HIR node model.
2. `lir_printer`
   Print the generated LIR as LLVM IR, preserving current output behavior.

This keeps the current LLVM IR path working, while creating an internal boundary that later allows non-LLVM backends to consume the same lowered program.

## Current Situation

Today `HirEmitter` does all of these at once:

- Walks HIR expressions/statements/functions/globals.
- Resolves control flow, labels, temporaries, storage, and ABI-specific details.
- Tracks per-function lowering state such as temp names, local slots, VLA state, and label mapping.
- Emits final LLVM IR strings directly.

That means the real lowering boundary already exists conceptually, but it is implicit and encoded as string-building helpers plus mutable state.

Relevant code:

- `src/codegen/llvm/llvm_codegen.cpp`
  Current top-level entry directly instantiates `HirEmitter`.
- `src/codegen/llvm/hir_emitter.hpp`
  `FnCtx`, temp creation, label handling, lvalue/rvalue lowering, statement lowering, and LLVM emission are mixed together.
- `src/frontend/hir/ir.hpp`
  Existing HIR already has stable IDs, typed expressions/statements, blocks, functions, globals, and struct layout metadata. This should remain the frontend IR contract.

## Design Principles

### 1. Keep HIR unchanged as frontend IR

Do not force backend-oriented details back into `hir::Module`.

HIR should continue to represent:

- typed expressions and statements
- frontend CFG/block structure
- source-facing entities and stable IDs
- layout metadata already computed by frontend

LIR should represent:

- explicit temporaries/results
- explicit loads/stores/branches/calls
- explicit block labels and terminators
- target-facing primitive operations

### 2. First split behavior, then generalize

The first refactor should be behavior-preserving, not a redesign of the compiler pipeline.

Initial success criterion:

- same CLI behavior
- same LLVM IR semantics
- ideally only trivial textual diffs

### 3. LIR must be inspectable

LIR should be a real node-based structure, not just `vector<string>`.

That gives:

- a stable boundary for future custom backends
- easier debugging and diffing
- a place for later target-independent lowering passes

### 4. LLVM-specific text formatting belongs only in printer

Anything that exists only because LLVM textual IR wants a specific spelling should live in `lir_printer`, not `hir_to_lir`.

Examples:

- textual `declare` formatting
- `%t42` naming policy
- LLVM syntax for `phi`, `gep`, `switch`, `call`, intrinsics, and attributes

## Proposed Architecture

### Files

Suggested end state:

- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/lir_printer.hpp`
- `src/codegen/lir/lir_printer.cpp`
- `src/codegen/lir/hir_to_lir.hpp`
- `src/codegen/lir/hir_to_lir.cpp`
- `src/codegen/llvm/llvm_codegen.cpp`

Optional transitional files:

- `src/codegen/llvm/lir_to_llvm_printer.hpp`
- `src/codegen/llvm/lir_to_llvm_printer.cpp`

If you want to keep LLVM-specific ownership clearer at first, `lir_printer` can stay under `src/codegen/llvm/` for phase 1 and move later.

### Top-level flow

Current:

`HIR -> HirEmitter -> LLVM IR string`

Target:

`HIR -> HIRToLIR -> LIR Module -> LIRPrinter(LLVM flavor) -> LLVM IR string`

In `src/codegen/llvm/llvm_codegen.cpp`, the new shape should become:

1. lower HIR module to LIR module
2. print LIR module as LLVM IR
3. return resulting string

## LIR Scope: Minimal Viable Model

The first LIR should be intentionally narrow: only enough to faithfully represent what `hir_emitter.cpp` already lowers.

### LIR module

- type declarations needed by codegen output
- globals
- function declarations
- function definitions
- interned constants/string objects
- required runtime/intrinsic declarations

### LIR function

- symbol name
- linkage/attrs needed by backend
- params
- blocks in deterministic order
- local frame objects / stack slots
- temporary value namespace

### LIR block

- block label/id
- ordered list of non-terminator instructions
- exactly one explicit terminator

### LIR values

Use explicit IDs instead of LLVM textual names.

Examples:

- `LirValueId`
- `LirBlockId`
- `LirStackSlotId`
- `LirGlobalId`

The LLVM printer can map those IDs to `%tN`, `%lv.x`, `@g`, and labels.

### LIR instructions

Start with operations already implicit in `HirEmitter`:

- constant materialization
- address-of global/local/string
- load / store
- integer and floating arithmetic
- comparisons
- casts / bitcasts / pointer casts
- gep/member/index address calculation
- call
- select
- inline asm
- vaarg helpers / intrinsic ops
- memcpy / stacksave / stackrestore wrappers if needed

### LIR terminators

- `br`
- `cond_br`
- `ret`
- `switch`
- `indirect_br`
- `unreachable` if useful for normalization

## Important Boundary Decisions

### A. Keep C type info attached in LIR

Do not lower immediately to raw LLVM types only.

Each LIR value or instruction should retain enough type information to support:

- future non-LLVM backends
- correct signedness-sensitive lowering
- bitfield handling
- aggregate/member/index lowering

Practical recommendation:

- keep `TypeSpec` or a compact backend-facing wrapper around it in LIR
- derive LLVM textual types only inside printer or printer helpers

### B. Lower HIR expressions to explicit evaluation steps

Current code often returns a string representing an LLVM SSA value while mutating context. In LIR, replace that with:

- emit zero or more LIR instructions
- return a `LirValueRef`

This is the single biggest conceptual cleanup.

### C. Separate frame objects from loads/stores

Current `FnCtx` mixes local-slot naming and storage semantics.

LIR should represent:

- stack objects / allocas / VLA allocations as data objects
- explicit `load` / `store` operations against those objects

This is critical for later non-LLVM backends.

### D. Make control-flow explicit

Current HIR already has blocks, but `HirEmitter` still performs block-label plumbing and terminator suppression through `last_term`.

LIR should require:

- every basic block ends with an explicit terminator
- block successors are encoded by terminators, not printer side effects

## Mapping From Current `HirEmitter`

The easiest path is not "rewrite everything", but "extract the hidden LIR already present".

### What moves into `hir_to_lir`

- `FnCtx` evolution into lowering context
- temp allocation
- block/label creation
- lvalue lowering
- rvalue lowering
- statement lowering
- control-flow construction
- local/global/function dedup logic
- intrinsic requirement tracking
- string pool ownership

### What stays printer-side

- final LLVM textual syntax
- instruction spelling
- declaration formatting
- `%` / `@` / label naming policy
- LLVM-specific details that are just rendering choices

### What should be split into helpers

- type conversion helpers
- struct/field layout lookup
- constant initializer emission
- ABI-specific vararg helpers
- inline asm constraint rewriting

Those helpers may still be shared, but should no longer write raw LLVM text unless they are explicitly printer helpers.

## Incremental Refactor Stages

### Stage 0: Mechanical prep

- Create `hir_to_lir` and `lir_printer` skeleton files.
- Rename `HirEmitter` mentally as "lowering + printer combined".
- Introduce a small internal `LirModule` type without changing behavior.

Exit criterion:

- build still passes
- `llvm_codegen.cpp` still returns LLVM IR using old path

### Stage 1: Capture emitted structure instead of strings

Replace these direct string sinks:

- `alloca_lines`
- `body_lines`
- preamble string assembly
- function body string assembly

with structured LIR containers first.

This stage should still allow printer code to be embedded in the same class temporarily.

Exit criterion:

- internal representation is no longer just `vector<string>`
- final output still produced from same class

### Stage 2: Split printer out

Introduce `LirPrinter` that consumes `LirModule` and produces LLVM IR string.

At this point:

- `HIRToLIR` owns all lowering state
- `LirPrinter` is stateless or mostly stateless rendering logic

Exit criterion:

- `llvm_codegen.cpp` no longer includes `hir_emitter.hpp`
- old `HirEmitter` can be removed or reduced to adapter glue

### Stage 3: Normalize special cases into LIR ops

Move currently ad hoc special logic into explicit LIR forms where useful:

- intrinsics
- memcpy
- varargs helpers
- computed goto / blockaddress
- bitfield extract/insert
- VLA stack save/restore

Exit criterion:

- printer contains less semantic branching
- future backends can consume these ops without parsing LLVM-like strings

### Stage 4: Add LIR inspection tooling

Add:

- `lir_printer` for debug dump, or
- simple textual dump of `LirModule`

This is useful before any non-LLVM backend exists.

Exit criterion:

- developers can diff HIR dump vs LIR dump vs LLVM IR output

## Recommended First Cut of LIR Data Structures

Not full code, but enough shape to guide implementation:

- `LirModule`
- `LirGlobal`
- `LirFunction`
- `LirBlock`
- `LirInst`
- `LirTerminator`
- `LirValue`
- `LirStackObject`
- `LirStringConst`
- `LirExternDecl`

For `LirInst`, prefer a variant-based design similar to HIR:

- `LirLoad`
- `LirStore`
- `LirBinary`
- `LirCast`
- `LirCmp`
- `LirCall`
- `LirGep`
- `LirSelect`
- `LirInlineAsm`
- `LirIntrinsic`
- `LirBitfieldExtract`
- `LirBitfieldInsert`

For `LirTerminator`:

- `LirBr`
- `LirCondBr`
- `LirSwitch`
- `LirRet`
- `LirIndirectBr`

## Naming and Ownership Recommendations

### Do not name it `hir_emitter` anymore

That name implies direct text emission. After split:

- `HIRToLIR` lowers
- `LIRPrinter` prints

### Keep LIR target-neutral in namespace if possible

Prefer:

- `tinyc2ll::codegen::lir`

instead of baking LLVM into the LIR namespace.

LLVM-specific printer can still live under:

- `tinyc2ll::codegen::llvm_backend`

## Risks

### 1. Textual output churn

Even if semantics stay identical, printer extraction may reorder:

- declarations
- string pool entries
- block emission order
- temporary numbering

Mitigation:

- define ordering rules early
- preserve current traversal order exactly in first pass

### 2. Semantic leakage into printer

If printer has to "repair" malformed or partial LIR, the split failed.

Mitigation:

- require complete terminators in LIR
- ensure type/legalization decisions happen before printing

### 3. Over-designing LIR too early

Trying to solve future register allocation, SSA optimization, and multi-target lowering now will slow the refactor.

Mitigation:

- first LIR should be a serialization of current lowering decisions
- postpone optimization concerns

### 4. Bitfields / VLAs / varargs / inline asm

These are the highest-risk areas because current code likely mixes semantics and LLVM details most tightly there.

Mitigation:

- migrate them last
- isolate them behind explicit LIR ops or focused helper APIs

## Validation Strategy

For each stage, validate both build stability and output parity.

Suggested checks:

1. Build `c4cll`.
2. Generate LLVM IR for a fixed corpus.
3. Compare before/after output.
4. If textual diffs exist, confirm semantic equivalence with `clang` or existing execution tests.

Suggested focus corpus:

- simple arithmetic/functions
- structs/unions/bitfields
- arrays and pointer arithmetic
- control flow and switch
- globals and initializers
- varargs
- inline asm
- computed goto if already supported
- VLA cases

If you later want formal regression gating, use the existing regression-guard workflow separately. It does not need to block this planning stage.

## Concrete Implementation Order

Recommended order inside the code:

1. Introduce `LirModule` and `LirFunction` containers.
2. Replace `FnCtx.body_lines` and `alloca_lines` with typed instruction/block storage.
3. Convert helper return values from `std::string` SSA names to `LirValueRef`.
4. Move final LLVM formatting into `LirPrinter`.
5. Move global/preamble/declaration emission into printer.
6. Extract special ops for bitfields, VLAs, varargs, and inline asm.
7. Add optional LIR debug dump.

This order minimizes simultaneous semantic change.

## Definition of Done

The refactor is successful when:

- `src/codegen/llvm/llvm_codegen.cpp` is a simple orchestration layer
- HIR remains the frontend IR contract
- lowering produces a structured LIR module
- LLVM IR output is produced exclusively by `lir_printer`
- current tests pass without newly failing cases
- a future backend can consume LIR without depending on LLVM textual conventions

## Short Recommendation

The right first milestone is not "design the perfect LIR". It is:

"Make the current hidden low-level lowering explicit, structured, and printer-independent."

If you keep that constraint, this refactor stays tractable and does not destabilize the frontend.
