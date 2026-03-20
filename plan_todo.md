# Codegen Refactor Todo

## Status

In progress.

The current repository is in a transitional state:

- `src/codegen/lir/ir.hpp` exists
- `src/codegen/lir/lir_printer.cpp` exists
- `src/codegen/lir/hir_to_lir.cpp` is still a stub
- `src/codegen/llvm/hir_emitter.cpp` still owns the real lowering work

So the key unfinished task is not "invent LIR", but:

- move lowering ownership from `hir_emitter.cpp` into `hir_to_lir.cpp`


## Phase 0: Re-baseline The Plan

### Objective

Replace the false "already complete" narrative with concrete, verifiable work items.

### Tasks

1. mark current state accurately

- LIR data model exists
- printer exists
- lowering split is not complete

2. define completion test

- `hir_to_lir.cpp` must build non-empty `LirModule`
- new path must be selectable independently

### Exit Criteria

- plan and todo no longer claim the split is done


## Phase 1: Add New/Old Path Selection

### Objective

Make refactor-safe parallel execution possible.

### Tasks

1. add CLI/backend selection

- `--codegen=legacy`
- `--codegen=lir`
- `--codegen=compare`

2. thread the selection into codegen entry

- route legacy to `HirEmitter`
- route new path to `hir_to_lir` + `lir_printer`

3. optional follow-up: add `src/apps/c4clle2e.cpp` if compare/debug flow becomes too noisy in `c4cll`

### Exit Criteria

- both codegen paths are runnable without source edits


## Phase 2: Implement Compare Mode

### Objective

Create a fast verification loop for behavior-preserving refactor work.

### Tasks

1. compare mode emits two outputs

- legacy `.ll`
- lir `.ll`

2. report textual diff

- non-zero exit on unexpected diff
- readable diff summary in CLI output

3. support deterministic output locations / naming

- stable temp filenames or explicit output prefix

### Exit Criteria

- one command can show new-vs-old codegen differences


## Phase 3: Move Module-Level Lowering

### Objective

Give `hir_to_lir.cpp` real ownership without first moving every expression helper.

### Tasks

1. lower globals in `hir_to_lir.cpp`
2. lower function list / dedup policy in `hir_to_lir.cpp`
3. move string pool ownership into `hir_to_lir.cpp`
4. move extern declaration accumulation into `hir_to_lir.cpp`
5. move intrinsic requirement flags into `hir_to_lir.cpp`

### Exit Criteria

- `lower(const hir::Module&)` returns a meaningful `LirModule`
- `HirEmitter::emit()` is no longer assembling full module state for the new path


## Phase 4: Move Function Skeleton Lowering

### Objective

Move the control skeleton before the leaf details.

### Tasks

1. move `FnCtx` or equivalent lowering state to LIR layer
2. move block allocation and label generation
3. move terminator construction

- return
- br
- cond br
- switch
- indirectbr

4. move alloca hoisting

### Exit Criteria

- LIR path can construct complete function CFG shells


## Phase 5: Move Expression/Statement Lowering By Risk Slice

### Objective

Avoid one massive unsafe port.

### Slice A: low-risk expression forms

- int / float / char / string literals
- decl refs
- simple casts
- simple unary/binary ops

### Slice B: storage and address forms

- lvalue lowering
- load/store
- member gep
- index gep

### Slice C: statement and control flow

- expr stmt
- local decl
- return
- if / while / for / do-while
- logical short-circuit
- ternary

### Slice D: backend-sensitive cases

- calls
- varargs
- memcpy / va_start / va_end / va_copy
- stacksave / stackrestore
- bitfields
- inline asm

### Exit Criteria

- each slice has compare-mode validation before the next slice starts


## Phase 6: Shrink Legacy Surface

### Objective

Prevent `hir_emitter.cpp` from remaining the de facto source of truth forever.

### Tasks

1. label migrated functions in `hir_emitter.cpp` as legacy-only
2. remove new-path-only responsibilities from `HirEmitter`
3. ensure `llvm_codegen.cpp` new path does not instantiate `HirEmitter`

### Exit Criteria

- new path can run end-to-end without depending on legacy lowering internals


## Phase 7: Regression Coverage

### Objective

Make refactor progress measurable.

### Tasks

1. create compare-mode smoke suite

- scalar arithmetic
- globals
- calls
- control flow
- switch
- indirectbr
- varargs
- bitfields
- namespace-heavy functions

2. gate migration slices on

- no crash
- no unexpected diff
- no regression in existing tests under legacy mode

3. add at least one end-to-end container/iterator case in compare mode

### Exit Criteria

- every migration slice has a stable regression guard


## Phase 8: Prepare LLVM API Backend

### Objective

Make the next step obvious once LIR lowering is real.

### Tasks

1. define backend interface on top of LIR

- text backend
- LLVM API backend

2. identify LIR fields missing for API-based emission
3. keep compare mode available even after API emitter appears

### Exit Criteria

- LIR is clearly the contract for both textual and API emitters


## Immediate Next Work

These should happen first, in order:

1. add `--codegen=legacy|lir|compare`
2. wire `llvm_codegen.cpp` so the new path calls `hir_to_lir.cpp`
3. implement compare mode with two `.ll` outputs
4. move module-level `LirModule` assembly out of `HirEmitter::emit()`
5. add the first compare-mode smoke tests


## Definition Of Done

This todo is complete when:

- `hir_to_lir.cpp` is the real lowering owner for the new path
- `hir_emitter.cpp` is only the legacy path
- new and old codegen paths can be switched by flag
- compare mode can emit and diff two `.ll` files
- the new path is stable enough to serve as the base for a future LLVM API emitter
