# RV64 String Literals And Extern Calls

## Goal

Implement a focused RV64 foundation for string constant storage/address
materialization and direct external-call lowering, using the c-testsuite
`string_literals_and_extern_calls` bucket as evidence without turning the work
into a broad c-testsuite sweep.

## Why This Exists

The undefined-main triage found 59 cases in the secondary
`string_literals_and_extern_calls` bucket. These cases currently also suffer
from the shared `.text`-only output-contract failure, but the prepared evidence
shows a separate feature gap: string constants need data emission and address
materialization, and many cases call libc or other extern functions.

Representative control:

```c
int strlen(char *);
int main() { char *p; p = "hello"; return strlen(p) - 5; }
```

For `src/00025.c`, prepared BIR records a direct extern `strlen` call, a local
slot for `p`, and string-constant address materialization for `.str0`.

Evidence to reuse:

- `build/rv64_c_testsuite_probe_v2/representative_evidence.md`
- `build/rv64_c_testsuite_probe_v2/classification.md`
- `build/rv64_c_testsuite_probe_v2/repair_order.md`
- `build/rv64_c_testsuite_probe_v2/classification_work/buckets/string_literals_and_extern_calls.txt`

## In Scope

- Emit RV64 data for string constants with stable labels and byte contents.
- Materialize addresses of string constants in prepared RV64 code.
- Lower direct fixed-arity extern calls needed by narrow string/libc cases.
- Preserve ABI-relevant argument and return-value behavior for the focused
  external-call subset.
- Add focused backend tests that prove string address materialization and a
  direct extern call without depending on the full 59-case bucket.

## Out Of Scope

- General libc conformance or broad c-testsuite pass-count chasing.
- Aggregate, pointer, floating, or scalar global storage support unrelated to
  string constants.
- Variadic calls unless a later idea scopes them explicitly.
- Reclassifying c-testsuite cases as unsupported in place of implementing the
  targeted lowering.
- Repairing the shared `.text`-only output contract; that belongs to the
  `rv64_text_only_fail_closed` idea and should land first.

## Candidate Cases

- Minimal representative: `src/00025.c`
- Additional bucket candidates:
  - `src/00026.c`
  - `src/00056.c`
  - `src/00058.c`
  - `src/00112.c`
  - `src/00125.c`
  - `src/00131.c`
  - `src/00132.c`
  - `src/00137.c`
  - `src/00138.c`
- Full secondary bucket:
  `build/rv64_c_testsuite_probe_v2/classification_work/buckets/string_literals_and_extern_calls.txt`

## Acceptance Criteria

- A narrow RV64 backend test proves string constant data emission with a stable
  label and address materialization in generated code.
- A narrow RV64 backend test proves a direct fixed-arity extern call using a
  string pointer argument and integer return value.
- `src/00025.c` progresses past the old missing-string-address/direct-call
  failure mode after the output-contract fix has landed.
- The implementation handles the mechanism generically for prepared string
  constants and direct extern calls, not just one c-testsuite filename.
- The proof records the exact subset run and does not require registering the
  full 220-case c-testsuite sweep as mandatory coverage.

## Closure Notes

Closed after the active runbook completed Steps 1-6. Focused RV64 backend
coverage includes `backend_codegen_route_riscv64_external_string_literal_strlen_direct_call`,
which requires string bytes, label address materialization, local pointer
store/reload, `mv a0`, `call strlen`, return capture, subtraction, and final
`mv a0`.

Representative recheck evidence in `build/rv64_string_extern_recheck/summary.md`
showed all ten selected `string_literals_and_extern_calls` cases emitted
assembly. The minimal representative `src/00025.c` showed `.str0`, the
`hello` byte sequence, `.text`, `main:`, `lla t0, .str0`, and `call strlen`,
so it progressed past the old missing string-address/direct-call failure mode.

Close-gate proof used the matching RV64/RISC-V backend subset in
`test_before.log` and `test_after.log`: `34 passed, 1 failed, 35 total` before
and after, with no new failures. The only failing test remained the known
pre-existing `backend_riscv_prepared_edge_publication`.

## Reviewer Reject Signals

- The patch matches `"hello"`, `strlen`, `.str0`, or specific c-testsuite
  filenames instead of lowering prepared string constants and direct extern
  calls generically.
- It claims progress by changing expected output, marking supported-path cases
  unsupported, or weakening call/string tests without explicit user approval.
- It hides unresolved string-address materialization behind the old
  `.text`-only failure or another generic linker error.
- It combines unrelated aggregate, pointer, floating, or broad global-storage
  work into this route and makes the string/call capability impossible to
  review independently.
- It only updates classification or helper names while retaining missing string
  data, missing address materialization, or broken direct-call ABI behavior.
