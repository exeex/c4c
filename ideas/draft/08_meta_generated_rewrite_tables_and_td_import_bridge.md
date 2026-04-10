# Meta-Generated Rewrite Tables And LLVM .td Import Bridge

Status: Draft
Last Updated: 2026-04-10

## Goal

Bootstrap a frontend-owned rewrite-rule system by expressing rewrite patterns in
C++-native syntax, generating `.h` / `.def` matcher tables with `@meta` during
c4c compile time, and providing a Python bridge that imports a practical subset
of LLVM `.td` DAG patterns into the new format.

## Why This Idea Exists

LLVM's `.td` pattern language is compact and battle-tested, especially for DAG
instruction-selection and rewrite rules. But it lives outside the host
language, which means:

- pattern authors learn and maintain a separate DSL
- rule collection and code generation happen in a separate toolchain
- c4c cannot use its own compile-time system to build its rewrite tables

At the same time, fully replacing TableGen in one step is unnecessary. A more
practical short-term path is:

1. build `@meta`
2. define a C++-native pattern DSL
3. use c4c compile time to generate rule tables
4. emit `.h` / `.def` output that GCC/Clang can consume today
5. import useful LLVM `.td` subsets with a Python bridge

That gives a real self-hosting loop long before the entire compiler stack is
native.

## Main Objective

Create a staged bootstrap path for rule-driven instruction selection / DAG
rewrite:

- authors describe rules in C++ template syntax
- c4c `@meta` collects and lowers them into generated matcher tables
- generated tables are emitted as checked-in or generated `.h` / `.def`
  artifacts
- a Python importer can translate LLVM `.td` `Pat` subsets into the same
  internal representation

## Core Closed Loop

The intended short-term closed loop is:

1. c4c compiles rewrite-rule sources
2. `@meta` collects pattern declarations
3. c4c compile time emits `generated_patterns.def` or `generated_matcher.h`
4. GCC/Clang compiles the generated artifacts as part of the c4c build
5. c4c gains a real compile-time code-generation role in its own optimizer

This already counts as a meaningful bootstrap milestone even if the final
self-hosted backend is not complete yet.

## Proposed Pattern DSL Shape

The new rule system should look like native C++ types instead of a separate
TableGen language.

Example direction:

```cpp
template<typename Match, typename Replace>
struct Pat;

template<typename RegClass, typename NameTag>
struct Bind;

template<typename X, typename Y>
struct add_;

template<typename X, typename Y>
struct shl_;

template<typename X, typename Y>
struct or_;

template<typename X, typename C, typename K>
struct FOO;
```

A rule can then look like:

```cpp
using x = Bind<GPR, x_tag>;
using c = Bind<UImm5, c_tag>;
using k = Bind<UImm32, k_tag>;

using P = Pat<
  or_<shl_<x, c>, k>,
  FOO<x, c, k>
>;
```

## Pattern Collection Strategy

Short term, the simplest compile-time registry is an include-driven type list:

```cpp
using TargetPatterns = type_list<
#include "patterns.def"
>;
```

Where `patterns.def` contains entries such as:

```cpp
Pat<or_<shl_<x, c>, k>, FOO<x, c, k>>,
Pat<add_<x, y>, ADDrr<x, y>>,
```

This provides:

- a compile-time registry
- no runtime singleton registration
- no global constructors
- easy transition to future reflection-based collection

Later, `@meta` or reflection can replace the include list and discover
`[[pattern]]` declarations automatically.

## Role Of `@meta`

`@meta` is the right tool for the next stage because the rewrite system needs
more than type-level expression. It needs global rule-set compilation:

- enumerate all patterns
- inspect their DAG structure
- factor common prefixes
- emit efficient matcher trees or matcher tables
- write generated `.h` / `.def` files

Templates are a good way to represent rules. `@meta` is the right way to
compile the full rule set into an efficient executable matcher.

## Generated Output

The first generated artifacts should be text formats that fit existing build
flows:

- `generated_patterns.def`
- `generated_matcher.h`

Possible contents:

- flat normalized rule lists
- prefix-factored matcher decision trees
- target feature / predicate tables
- operand-class metadata

The output should be deterministic and diff-friendly so changes are easy to
review.

## Python LLVM `.td` Import Bridge

To avoid rebuilding a pattern ecosystem from zero, add a Python importer for a
practical LLVM `.td` subset.

The importer should:

1. read selected `.td` files
2. extract a supported subset of:
   - `def : Pat<...>`
   - `PatFrag`
   - `PatLeaf`
   - `Requires<[...]>`
3. normalize them into a simple intermediate representation
4. emit the c4c-native DSL or generated `.def`

The goal is not full TableGen compatibility. The goal is a bootstrap bridge for
the common DAG-pattern subset that matters most.

## Suggested Initial Import Scope

Start with the most mechanical subset:

- one-line `def : Pat<lhs, rhs>;`
- simple operand binders such as `GR32:$src`, `addr:$dst`
- simple nested DAGs
- trailing `Requires<[...]>`
- target-local fragments used by a small chosen rule family

Good first targets:

- basic X86 loads/stores
- simple atomic load/store patterns
- constant materialization patterns
- small AArch64 arithmetic/load patterns

## Intermediate Representation For The Importer

The Python bridge should not emit C++ directly from raw regex captures. It
should first normalize into a small IR, for example:

```python
Pat(
  lhs=DAG("atomic_store_32", [Binder("GR32", "src"), Binder("addr", "dst")]),
  rhs=Instr("MOV32mr", [Binder("addr", "dst"), Binder("GR32", "src")]),
  predicates=[]
)
```

This IR can then emit:

- C++ template DSL
- `.def`
- generated matcher tables

without rewriting the parser.

## Concrete Injection Points

Short-term files or directories that likely own this work:

- `src/` area that will host the future pattern DSL headers
- `scripts/` for the Python `.td` importer
- `ref/llvm-project/llvm/lib/Target/...` as importer source material
- generated files under a future `generated/` or target-specific include path

Suggested first generated files:

- `generated/x86_patterns.def`
- `generated/aarch64_patterns.def`
- `generated/x86_matcher.h`

## Validation

- importer round-trip tests for the supported `.td` subset
- generated `.def` / `.h` diff tests for determinism
- compile tests proving GCC/Clang can consume the generated artifacts
- target-local matcher tests on a small imported rule family
- compile-time tests proving c4c `@meta` can emit the generated output

## Constraints

- do not attempt full LLVM TableGen compatibility in the first slice
- do not block on full self-hosting before validating the rule system
- keep the generated output textual and reviewable at first
- keep the first importer narrow enough that failures are obvious and debuggable

## Non-Goals

- no complete replacement of LLVM TableGen in one step
- no requirement that the importer understand every `.td` metaprogramming trick
- no runtime rule registry in the first bootstrap slice
- no requirement that `@meta` immediately replace the Python importer
