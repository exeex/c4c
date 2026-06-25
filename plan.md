# RV64 Object Route Data Sections Runbook

Status: Active
Source Idea: ideas/open/357_rv64_object_route_data_sections_globals_strings.md

## Purpose

Activate the next prepared-object repair after diagnostics/text-route work: RV64
object emission for prepared-module data that is already present in the shared
prepared contract.

## Goal

Emit correct RV64 ELF data sections, symbols, and relocations for supported
prepared globals and string constants without inventing upstream data semantics
inside target object emission.

## Core Rule

Only consume data representation that the prepared module already publishes.
If string literals, globals, initializer bytes, alignment, or address-use
semantics are missing upstream, stop and route that as a BIR/LIR prepared-data
contract problem instead of patching it locally in RV64 object emission.

## Read First

- ideas/open/357_rv64_object_route_data_sections_globals_strings.md
- review/354_prepared_shape_classification.md when available
- tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake
- scripts/check_progress_rv64_gcc_c_torture_backend.sh
- RV64 object emission and ELF writer surfaces that currently reject or omit
  prepared globals/string constants

## Current Targets

- Remove the supported-data portion of the blanket
  `prepared.module.globals/string_constants` rejection.
- Emit `.rodata` or data-equivalent sections for string constants.
- Emit data sections for supported scalar or byte-backed global initializers.
- Define stable data symbols with section placement, binding, size, and
  alignment from prepared metadata.
- Generate RV64 relocation records for code references to prepared data symbols.
- Prove representative string and global cases either execute or move to a
  later, more precise unsupported bucket.

## Non-Goals

- Do not complete missing LIR/BIR prepared-data representation here.
- Do not infer initializer bytes, symbol lifetime, or address-use meaning by
  scanning source, LIR, or BIR from RV64 object emission.
- Do not implement full C aggregate initializer support beyond what prepared
  data already represents.
- Do not weaken gcc torture expectations, skip cases, or downgrade supported
  paths to unsupported.
- Do not add testcase-name shortcuts for the representative cases.
- Do not require the heavy gcc torture scan in ordinary full CTest.

## Working Model

- Prepared data publication is the contract boundary.
- RV64 object emission owns target section layout, ELF symbols, and relocation
  emission for already-published prepared data.
- Missing prepared data facts are upstream contract gaps and should produce
  precise diagnostics or a follow-up idea, not target-local reconstruction.
- The internal object model and ELF writer should remain the route for emitted
  data so later data kinds can extend the same path.

## Execution Rules

- Keep each code slice semantic and general; avoid source-pattern or filename
  matching.
- Start from diagnostics/admission state so unsupported data cases fail with a
  specific reason.
- Add focused tests before relying on the heavy scan as proof.
- For code-changing steps, use a build or narrow test proof before handing back
  to the supervisor.
- Use this validation ladder as scope grows: build -> focused object/data tests
  -> narrow RV64 torture allowlist -> milestone RV64 torture scan.
- If a representative case lacks prepared data representation, document the
  precise upstream gap in `todo.md` and ask the supervisor to route it; do not
  mutate this source idea unless directed by lifecycle review.

## Ordered Steps

### Step 1: Inspect Prepared Data and Current Rejection Path

Goal: identify exactly what prepared globals/string constants are available to
RV64 object emission and where the current blanket rejection fires.

Primary target: RV64 object module construction and existing prepared-object
diagnostics/admission code.

Concrete actions:

- Locate the prepared module data structures visible to RV64 object emission.
- Trace how function bodies currently reference globals and string constants.
- Identify which metadata exists for section choice, symbol name, initializer
  bytes, alignment, mutability, size, and relocation target.
- Check representative cases:
  `src/20000112-1.c`, `src/20000223-1.c`, `src/20000224-1.c`, and
  `src/20000227-1.c`.
- Record any missing upstream prepared-data facts in `todo.md` rather than
  guessing them in target emission.

Completion check:

- The executor can name the current rejection sites, available prepared-data
  fields, and any missing upstream contract facts.
- A narrow proof command or diagnostic run is recorded in `todo.md`.

### Step 2: Add Target Data Section and Symbol Emission

Goal: emit object data sections and symbols for supported prepared strings and
globals.

Primary target: the internal object model / ELF writer path used by RV64 object
emission.

Concrete actions:

- Map prepared string constants to read-only data sections with correct bytes,
  size, alignment, and symbol definitions.
- Map supported global initializers to writable or read-only sections according
  to prepared metadata.
- Preserve symbol naming, binding, visibility, section index, and size in the
  object writer.
- Keep unsupported initializer forms behind precise diagnostics.
- Add or update focused object tests that inspect emitted sections/symbols.

Completion check:

- Focused object tests prove emitted sections and symbols are structurally
  valid.
- Unsupported prepared-data shapes produce a structured diagnostic instead of
  the old blanket module-shape rejection.

### Step 3: Add RV64 Code/Data Relocations

Goal: support code references to prepared data symbols through correct RV64 ELF
relocations.

Primary target: RV64 instruction/object relocation emission for address uses.

Concrete actions:

- Identify existing address materialization forms for globals and strings.
- Emit relocation records compatible with clang RV64 linking for symbol address
  uses already present in prepared code.
- Keep relocation kinds and addend handling explicit and test-covered.
- Avoid hard-coded text-only addresses or assumptions about one binary layout.

Completion check:

- Focused tests or object inspection prove code/data relocations are present
  and linkable.
- Representative string/global cases advance past data-symbol address use or
  fail at a later structured backend bucket.

### Step 4: Prove Representative String and Global Cases

Goal: show the dominant data bucket is repaired or accurately routed.

Primary target: RV64 gcc torture backend object runner.

Concrete actions:

- Build a temporary allowlist containing:
  `src/20000112-1.c`, `src/20000223-1.c`, `src/20000224-1.c`, and
  `src/20000227-1.c`.
- Run:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-data-allowlist.txt \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

- Confirm cases reach RV64 link and qemu execution, or move to a precise later
  unsupported bucket.
- Keep runtime mismatches, link failures, or later backend failures distinct
  from data-section repair.

Completion check:

- The narrow allowlist proof is recorded in `test_after.log` or the supervisor
  delegated proof artifact.
- The old blanket data prepared-module rejection is gone for supported scalar
  and string data.

### Step 5: Run Milestone Validation and Route Residuals

Goal: validate the repair beyond the representative set and preserve proper
layer boundaries for remaining failures.

Primary target: broader RV64 gcc torture backend scan and existing object
tests.

Concrete actions:

- Run existing object tests and c4c-as/c4c-objdump roundtrip tests selected by
  the supervisor.
- When the supervisor treats this as a milestone, run:

```sh
CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

- Compare residual prepared data failures against the acceptance criteria.
- Route missing prepared-data representation to an upstream BIR/LIR data-init
  idea instead of expanding RV64 emission scope.

Completion check:

- Existing relevant object and roundtrip tests remain green.
- Broader scan evidence shows the supported data bucket progressed without
  skips, expectation downgrades, or testcase-shaped matching.
- Any remaining data failures are classified as either unsupported prepared
  contract gaps or later backend buckets.
