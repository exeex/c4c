# Full RV64 Assembly/Object/Disassembly Roundtrip Runbook

Status: Active
Source Idea: ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md

## Purpose

Expand the minimal c4c assembler and objdump route into a complete RV64I plus
c4c EV64 custom instruction roundtrip path.

Goal: for the selected supported RV64 corpus, prove:

```text
input.s -> pass1.o -> pass1.s -> pass2.o -> pass2.s -> pass3.o
```

with `pass1.s == pass2.s` and `pass2.o == pass3.o`.

## Core Rule

Do not claim broad RV64 roundtrip progress from the existing `.insn.d`, `li`,
and `ret` subset. Each step must generalize parser, encoder, decoder, or proof
coverage for the declared RV64I corpus.

## Read First

- `ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md`
- `src/apps/c4c-as.cpp`
- `src/apps/c4c-objdump.cpp`
- RV64 assembler and encoder sources under `src/`
- backend CMake tests under `tests/backend/`

## Current Targets And Scope

- RV64I base integer instruction coverage.
- Common pseudoinstructions only when they lower to supported encodings or have
  a stable canonical spelling.
- Register aliases and canonical register printing policy.
- Signed and unsigned immediate parsing with range checks.
- Labels, local labels, branches, jumps, calls where in RV64I scope, and
  relocatable fixups required by the corpus.
- `.text` and the minimum directives needed by the corpus.
- c4c EV64 `.insn.d` encode/decode alongside ordinary RV64 instructions.
- CTest-integrated full roundtrip proof.

## Non-Goals

- Do not recover original C/C++ source.
- Do not preserve GNU assembler formatting, comments, or debug information.
- Do not silently accept unsupported extensions or directives.
- Do not rely on external assemblers or objdump as the source of truth.
- Do not treat M/A/F/D/C or other extensions as in scope unless the corpus and
  support declaration explicitly include them.

## Working Model

- `c4c-as` owns parsing supported assembly into c4c RV64 encodings and
  relocatable objects.
- `c4c-objdump` owns decoding c4c-produced RV64 ELF objects into canonical
  assembly.
- The test corpus is a committed repo input and must be broad enough to cover
  every declared RV64I instruction family.
- Unsupported bytes, directives, encodings, or extensions must fail closed with
  diagnostics.

## Execution Rules

- Keep implementation slices narrow and prove each with build plus focused
  backend tests selected by the supervisor.
- Add or extend tests before claiming support for a new instruction family.
- Keep object stability proof separate from textual canonicalization proof.
- Prefer shared parser/encoder/decoder helpers over app-local special cases
  when a capability is needed by both inline asm and file asm routes.
- Preserve the c4c EV64 custom instruction path; it must not print as
  `<unknown>`.
- Escalate to broader backend or full CTest once multiple instruction families,
  fixups, or object-writing behavior are touched.

## Ordered Steps

### Step 1: Declare RV64I Corpus And Canonical Contract

Goal: commit the supported extension boundary and representative roundtrip
corpus before broad implementation starts.

Primary targets:

- `tests/backend/`
- new or existing RV64 `.s` corpus input
- CMake test registration scaffolding

Actions:

- Define the exact supported set for this idea: RV64I plus c4c EV64
  `.insn.d`, unless the executor and supervisor explicitly choose additional
  extensions.
- Add a committed assembly corpus covering all declared RV64I major families:
  arithmetic/logical register-register, register-immediate, loads, stores,
  branches, jumps, upper-immediate, PC-relative, shifts, comparisons, edge
  immediates, register aliases, labels, and `.insn.d`.
- Add test scaffolding that can run the intended
  `input.s -> pass1.o -> pass1.s -> pass2.o -> pass2.s -> pass3.o` pipeline,
  even if early assertions are limited to currently supported subsets.
- Keep unsupported extension coverage fail-closed.

Completion check:

- The corpus and canonicalization contract are visible in repo test files.
- Step 1 proof builds and runs the focused test target selected by the
  supervisor.
- `todo.md` records the exact proof command and current unsupported gaps for
  later steps.

### Step 2: Generalize RV64 Assembly Parsing And Encoding

Goal: extend `c4c-as` and shared RV64 assembly helpers from the minimal subset
to the declared RV64I corpus.

Primary targets:

- RV64 line parser and encoder sources
- `src/apps/c4c-as.cpp`
- backend assembler tests

Actions:

- Parse supported instruction mnemonics, register aliases, immediates, labels,
  and required directives.
- Encode RV64I instruction formats and preserve EV64 `.insn.d` handling.
- Add signedness and range diagnostics for immediates and shifts.
- Fail closed on unsupported directives, extensions, or malformed operands.
- Keep implementation semantic; do not add named-case shortcuts for the corpus.

Completion check:

- `c4c-as` assembles the committed corpus through every supported line and
  emits a relocatable object.
- Focused tests cover representative successes and failures for the newly
  supported instruction families.

### Step 3: Add Label, Branch, Jump, And Relocation Support

Goal: support the fixups needed by the selected corpus without weakening object
truthfulness.

Primary targets:

- RV64 assembler fixup and object emission code
- ELF relocation/object writer helpers
- branch and jump tests

Actions:

- Resolve local labels when a fixed PC-relative encoding is possible.
- Emit relocations required by supported unresolved RV64 forms.
- Validate forward and backward branch ranges and jump targets.
- Keep `.text` symbol and function boundaries truthful for objdump.

Completion check:

- Corpus branch, jump, call, label, and PC-relative cases assemble or fail with
  clear diagnostics according to the declared contract.
- Focused tests verify object text bytes and relocations where relevant.

### Step 4: Generalize RV64 Objdump Decoding And Canonical Printing

Goal: decode the RV64I corpus and print stable canonical assembly.

Primary targets:

- `src/apps/c4c-objdump.cpp`
- shared RV64 decode/print helpers if present or introduced
- backend objdump tests

Actions:

- Decode all declared RV64I instruction formats and EV64 `.insn.d`.
- Print canonical register names and immediate forms consistently.
- Choose stable pseudo versus real instruction spelling and document it in the
  test expectations.
- Fail closed on unknown bytes instead of skipping or guessing.

Completion check:

- `c4c-objdump` disassembles `pass1.o` from the committed corpus into canonical
  `.s`.
- Tests prove representative decode output for each declared instruction
  family plus unsupported-byte diagnostics.

### Step 5: Prove Full Two-Pass Roundtrip Stability

Goal: make the source idea acceptance proof part of regular CTest output.

Primary targets:

- backend CMake tests
- corpus roundtrip helper script or CMake driver
- canonical assembler and objdump outputs

Actions:

- Run the full pipeline:
  `input.s -> pass1.o -> pass1.s -> pass2.o -> pass2.s -> pass3.o`.
- Assert `pass1.s == pass2.s`.
- Assert `pass2.o == pass3.o`.
- Ensure no external assembler or objdump is used as the source of truth.
- Register the test in ordinary full `ctest --test-dir build -j
  --output-on-failure` output.

Completion check:

- The roundtrip test passes through regular CTest.
- The proof covers the committed RV64I plus EV64 corpus.
- `todo.md` names the full proof command and any broader validation result.

### Step 6: Closure Review And Broader Validation

Goal: verify the implementation satisfies the source idea rather than only the
latest runbook step.

Primary targets:

- `ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md`
- `plan.md`
- `todo.md`
- canonical regression logs

Actions:

- Compare the final diff and tests against the source idea acceptance criteria
  and reviewer reject signals.
- Confirm the implementation did not rely on external tools, testcase-shaped
  shortcuts, weakened unsupported contracts, or the minimal smoke subset.
- Run the supervisor-selected broader or full validation, normally including
  full CTest for milestone closure.
- Hand lifecycle closure back to the plan owner only after code and tests are
  acceptance-ready.

Completion check:

- Source acceptance criteria are satisfied.
- Regression guard can compare matching before and after logs for the close
  scope.
- The plan owner can close the idea without adding new implementation work.
