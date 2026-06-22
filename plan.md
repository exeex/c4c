# RV64 String Literals And Extern Calls Runbook

Status: Active
Source Idea: ideas/open/308_rv64_string_literals_and_extern_calls.md

## Purpose

Implement the focused RV64 foundation for string constant data emission,
string-address materialization, and direct fixed-arity extern-call lowering.

Goal: make the narrow string/libc representative path progress through generic
prepared string constants and direct extern calls, using `src/00025.c` as the
minimal c-testsuite control without turning this into a broad suite sweep.

## Core Rule

Lower prepared string constants and direct extern calls semantically. Do not
special-case `"hello"`, `strlen`, `.str0`, c-testsuite filenames, or exact
source text, and do not fold aggregate, pointer, floating, or broad global
storage work into this idea.

## Read First

- `ideas/open/308_rv64_string_literals_and_extern_calls.md`
- `build/rv64_c_testsuite_probe_v2/representative_evidence.md`
- `build/rv64_c_testsuite_probe_v2/classification.md`
- `build/rv64_c_testsuite_probe_v2/repair_order.md`
- `build/rv64_c_testsuite_probe_v2/classification_work/buckets/string_literals_and_extern_calls.txt`

If these probe artifacts are missing, regenerate only the narrow evidence
needed for this plan under `build/rv64_c_testsuite_probe_v2/` or another
non-root scratch location.

## Current Targets

- Minimal representative: `src/00025.c`
- Prepared BIR facts to preserve:
  - direct extern `strlen` call
  - local slot for `p`
  - string-constant address materialization for a stable string label such as
    `.str0`
- Narrow backend coverage for:
  - string constant data emission with stable labels and byte contents
  - address materialization for prepared string constants
  - direct fixed-arity extern call with a string pointer argument and integer
    return value
- Secondary bucket candidates for representative recheck:
  `src/00026.c`, `src/00056.c`, `src/00058.c`, `src/00112.c`,
  `src/00125.c`, `src/00131.c`, `src/00132.c`, `src/00137.c`,
  `src/00138.c`

## Non-Goals

- Do not pursue general libc conformance or broad c-testsuite pass-count
  chasing.
- Do not implement aggregate, pointer, floating, or scalar global storage
  unrelated to string constants.
- Do not implement variadic calls unless a later idea scopes them explicitly.
- Do not reclassify supported-path cases as unsupported instead of
  implementing the targeted lowering.
- Do not repair the shared `.text`-only output contract here; that route has
  already landed before this idea.
- Do not leave root-level scratch logs.

## Working Model

The previous output-contract repair should make prepared functions visible
instead of silently succeeding with `.text` only. This plan owns the next
feature gap exposed by the `string_literals_and_extern_calls` bucket: prepared
string constants need RV64 data emission and addresses, and direct fixed-arity
extern calls need ABI-correct argument and return-value handling for the
focused string/libc subset.

The minimal representative is:

```c
int strlen(char *);
int main() { char *p; p = "hello"; return strlen(p) - 5; }
```

Progress is capability repair, not classification churn or named-case
expectation edits.

## Execution Rules

- Work in small packets tied to one step at a time.
- Keep `todo.md` as the packet progress and proof log; do not churn this
  runbook for routine executor updates.
- Prefer owner-surface repairs in prepared-module data emission, string
  address materialization, and direct-call lowering over testcase-shaped
  checks.
- Preserve ABI-relevant argument placement and integer return-value behavior
  for the focused fixed-arity extern-call subset.
- For code-changing steps, prove with a fresh build or compile check plus the
  delegated narrow backend subset.
- Treat expectation downgrades, named string/call shortcuts, or unsupported
  reclassification as route failure.

## Ordered Steps

### Step 1: Inspect String And Extern-Call Lowering Gaps

Goal: identify the concrete RV64 owner surfaces that currently miss prepared
string data emission, string-address materialization, and direct extern-call
lowering.

Primary targets:

- RV64 prepared-module data emission path
- RV64 string constant representation and label assignment
- RV64 address materialization for prepared string constants
- RV64 direct-call lowering and fixed-arity ABI argument/return handling
- `src/00025.c` prepared BIR and generated RV64 output or diagnostics

Actions:

- Inspect the prepared BIR and current RV64 output for `src/00025.c` or an
  equivalent narrow fixture.
- Trace where string constants are recorded, where module data is emitted, and
  where string addresses should be materialized into registers or slots.
- Trace direct extern-call lowering for the fixed-arity call shape needed by
  `strlen(p)`.
- Record the smallest owner surfaces and the first semantic implementation
  target in `todo.md`.

Completion check:

- `todo.md` names the missing or incomplete owner surfaces for string data,
  string address materialization, and direct extern-call lowering.
- No implementation files need to change for this inspection-only step.

### Step 2: Emit String Constant Data

Goal: emit RV64 data for prepared string constants with stable labels and byte
contents.

Primary targets:

- RV64 prepared-module data emission owner files
- String constant label assignment and escaping/byte emission helpers
- Existing assembly output ordering and section-selection code

Actions:

- Emit string constants into the appropriate RV64 data section with stable
  labels derived from prepared string facts, not from testcase names.
- Preserve exact byte contents including terminators expected by the prepared
  string representation.
- Keep unrelated global storage forms out of this step.
- Add or update focused backend coverage for string data emission.

Completion check:

- A narrow backend test proves stable string labels and emitted byte contents.
- Fresh build or compile proof covers the touched backend path.

### Step 3: Materialize String Constant Addresses

Goal: make prepared string constant addresses usable by generated RV64 code.

Primary targets:

- RV64 address materialization route for prepared string constants
- Local slot assignment/store route used by `char *p; p = "hello";`
- Any relocation-friendly label-address sequence already used by RV64 output

Actions:

- Lower string constant address values through a generic label-address
  materialization rule.
- Store or pass the resulting pointer through existing local-value machinery
  without string-specific testcase shortcuts.
- Extend focused backend coverage so address materialization is observable in
  generated RV64 code.

Completion check:

- A narrow backend test proves generated code materializes a string label
  address and can move it through the expected local or call path.
- The implementation does not depend on `"hello"`, `.str0`, `strlen`, or a
  c-testsuite filename.

### Step 4: Lower Direct Fixed-Arity Extern Calls

Goal: lower the direct extern-call subset needed by narrow string/libc cases
with ABI-relevant argument and return-value behavior.

Primary targets:

- RV64 direct-call instruction selection or emission path
- Fixed-arity call argument placement for pointer/integer arguments
- Integer return-value handling used by expressions such as `strlen(p) - 5`
- Existing unsupported-call diagnostics for out-of-scope call forms

Actions:

- Lower direct extern calls by symbol name from prepared call facts, not from
  source filenames or exact callee-specific shortcuts.
- Place supported fixed-arity arguments according to the RV64 ABI route used by
  the backend.
- Capture integer return values through the existing value path so downstream
  arithmetic works.
- Keep variadic calls and broad libc behavior out of scope.
- Add or update focused backend coverage for a direct extern call using a
  string pointer argument and integer return value.

Completion check:

- A narrow backend test proves a direct fixed-arity extern call receives a
  string pointer argument and exposes an integer return value.
- Unsupported call forms still fail clearly instead of being silently accepted
  or mislowered.

### Step 5: Recheck Representative Bucket Cases

Goal: verify the minimal representative and nearby bucket cases no longer
hide the old string-address/direct-call failure mode.

Primary targets:

- `src/00025.c`
- selected nearby candidates from the `string_literals_and_extern_calls`
  bucket
- the focused backend tests added by earlier steps

Actions:

- Re-run the narrow proof selected by the supervisor for `src/00025.c` and the
  focused backend tests.
- Recheck a small set of nearby bucket candidates only as representative
  evidence, not as mandatory full-bucket coverage.
- Record each representative's visible result or next failure mode in
  `todo.md`.
- Keep unrelated aggregate/global-storage failures as follow-up scope, not
  hidden work inside this plan.

Completion check:

- `src/00025.c` progresses past the old missing-string-address/direct-call
  failure mode after the output-contract fix.
- `todo.md` records the exact subset run and representative outcomes.

### Step 6: Closure Readiness Check

Goal: decide whether the source idea is complete and ready for supervisor
review.

Actions:

- Verify focused string data, address materialization, and direct extern-call
  tests are green.
- Verify `src/00025.c` no longer exhibits the old missing string address or
  broken direct-call ABI behavior.
- Verify no tests or expectations were weakened.
- Verify no aggregate, pointer, floating, scalar-global, variadic-call, broad
  libc, or full c-testsuite sweep work was mixed into this route.

Completion check:

- The active runbook has proof for all acceptance criteria in the source idea,
  or `todo.md` clearly records the remaining blocker for plan-owner review.
