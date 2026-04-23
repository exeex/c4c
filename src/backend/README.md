# Backend Architecture Contract

`src/backend/` is the architecture contract for the compiler backend. It owns
the shared lowering pipeline from LIR or BIR input to target-local backend
output, and it defines which responsibilities are shared backend truth versus
target-local consumption.

The current C++ port is intentionally split into three broad layers:

- `bir/`: lowers typed LIR into backend-owned BIR and prints / validates that
  IR
- `prealloc/`: prepares semantic BIR into prepared backend facts such as stack
  layout, addressing, liveness, and regalloc publications
- `mir/<target>/`: target-local consumers that turn prepared backend facts into
  target-specific output

The public backend entry surface is declared in
[`backend.hpp`](/workspaces/c4c/src/backend/backend.hpp:1) and implemented in
[`backend.cpp`](/workspaces/c4c/src/backend/backend.cpp:1).

---

## Pipeline

```text
LIR Module or BIR Module
        |
        v
backend::emit_module / dump_module
        |
        +--> bir/
        |    lower LIR to backend-owned BIR when needed
        |
        +--> prealloc/
        |    prepare semantic BIR into:
        |    - prepared names
        |    - control flow
        |    - value locations
        |    - stack layout
        |    - addressing
        |    - liveness
        |    - regalloc
        |
        +--> mir/<target>/
             consume prepared facts through target-local codegen
             and debug / route surfaces
```

For x86 specifically, the current public backend flow is:

1. Normalize the input in `backend.cpp`.
2. Lower LIR to BIR when the caller starts from LIR.
3. Run semantic prepare through `prepare_semantic_bir_module_with_options(...)`.
4. Hand the resulting `PreparedBirModule` to `mir/x86/api/api.cpp`.
5. Route into the x86 module emitter.

The backend dump path is parallel to emission:

- generic semantic / prepared BIR dumps stay owned by shared backend code
- target-local MIR summary / trace dumps are owned by the x86 route-debug
  surface

---

## Directory Layout

```text
src/backend/
  backend.hpp              public backend entrypoints and dump options
  backend.cpp              shared module-entry orchestration
  CMakeLists.txt           backend library build manifest

  bir/                     backend-owned IR and LIR-to-BIR lowering
    bir.hpp                core BIR structs
    bir.cpp                BIR helpers
    bir_printer.*          BIR text printer
    bir_validate.*         BIR validation
    lir_to_bir*.cpp        semantic lowering from LIR into BIR

  prealloc/                prepared backend facts and preallocation pipeline
    prealloc.hpp           PreparedBirModule and canonical prepared data model
    prealloc.cpp           prepare pipeline entry
    legalize.cpp           legality / normalization
    liveness.cpp           prepared liveness
    regalloc.cpp           prepared regalloc publication
    stack_layout.cpp       stack-layout driver
    target_register_profile.* target-specific register profile support
    stack_layout/          stack slot assignment and support passes

  mir/
    aarch64/               target-local backend route
    riscv/                 target-local backend route
    x86/                   target-local x86 backend route
```

---

## Ownership Rules

- `backend.cpp` may orchestrate shared preparation and route into a target
  entry surface, but it should not depend on target-private implementation
  files.
- `mir/<target>/` may consume prepared backend facts, but it should not reopen
  canonical BIR or prealloc ownership locally.
- target-local debug output should be observational and should not become a
  second execution path.
- markdown contracts under a target subtree are part of the active design
  surface when that target is being rebuilt; they are not just historical
  notes.

## Shared Responsibilities

### `bir/`

This layer owns the compiler-facing backend IR contract. It should answer:

- what the backend believes the program means
- which semantic facts have already been normalized before target-local codegen
- what errors belong to generic lowering rather than target-specific consumers

### `prealloc/`

This layer owns truthful publication of prepared backend facts. It should
publish canonical answers for:

- value homes
- stack slots and frame layout
- addressing forms
- control-flow structure
- liveness and regalloc assignments

Target-local codegen should consume these facts rather than reconstruct them.

### `mir/<target>/`

This layer owns target-specific consumption. It should:

- validate target-local assumptions
- turn prepared facts into target-local output
- keep debug and route-summary surfaces observational
- avoid reopening canonical prepared ownership locally

---

## Current X86 Direction

The x86 backend is in the middle of a contract-first redesign. The old
monolithic codegen tree has been extracted into in-place `*.cpp.md` /
`*.hpp.md` artifacts, while the live code is being re-laid out around explicit
ownership seams under `mir/x86/`.

That means two parallel artifact families exist today:

- live implementation files that define the new ownership graph
- in-place markdown companions that now serve one of two roles:
  - legacy evidence
  - design contract

The design intent is to make target-local consumption narrower and more honest:

- `api/` is the public target entry
- `module/` owns top-level module orchestration
- `abi/` owns stable machine-policy helpers
- `core/` owns shared emission data / buffer utilities
- `lowering/` owns canonical semantic families
- `prepared/` owns thin fast-path adapters
- `debug/` owns route-summary observation only

For x86-specific ownership and subsystem details, see
[src/backend/mir/x86/README.md](/workspaces/c4c/src/backend/mir/x86/README.md:1).
