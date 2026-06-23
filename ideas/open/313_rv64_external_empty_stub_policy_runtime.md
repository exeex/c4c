# RV64 External Empty Stub Policy And Runtime Calls

## Goal

Define and implement a narrow RV64 policy for external libc, libm, string, and
user-declared external calls so emitted programs either link to real runtime
implementations correctly or fail with an explicit unsupported diagnostic
instead of generating bodyless executable stubs.

## Why This Exists

The RV64 c-testsuite runtime triage classified 59 runtime rows as
`unresolved_external_or_empty_stub_emission`, the largest raw bucket. It was
ranked after the first backend repair families because this is not just a code
emission bug: it needs an explicit runtime/linker/builtin policy and negative
tests for unsupported external calls.

Step 4 evidence in
`build/rv64_c_testsuite_probe_latest/triage_step4/summary.md` describes the
mechanism as `.globl` labels for libc/libm/string/extern functions with no
body, so calls fall into empty labels or subsequent labels instead of real
implementations. `src/00025.c` is the timeout representative: it calls
`strlen`, then emitted assembly contains a bodyless `strlen:` label.

## Owned Feature Family

RV64 backend/runtime policy for unresolved external call emission, external
symbol declaration, call relocation/linkage, supported builtin or library-call
lowering, and explicit unsupported diagnostics for external calls that the
compiler cannot safely emit.

## Candidate Cases

Primary runtime candidates from the Step 4 classification:

- `src/00025.c`
- `src/00056.c`
- `src/00125.c`
- `src/00131.c`
- `src/00132.c`
- `src/00154.c`
- `src/00156.c`
- `src/00157.c`
- `src/00158.c`
- `src/00159.c`
- `src/00160.c`
- `src/00161.c`
- `src/00163.c`
- `src/00164.c`
- `src/00165.c`
- `src/00166.c`
- `src/00167.c`
- `src/00168.c`
- `src/00169.c`
- `src/00170.c`
- `src/00171.c`
- `src/00172.c`
- `src/00173.c`
- `src/00174.c`
- `src/00175.c`
- `src/00176.c`
- `src/00177.c`
- `src/00178.c`
- `src/00179.c`
- `src/00180.c`
- `src/00181.c`
- `src/00182.c`
- `src/00183.c`
- `src/00184.c`
- `src/00185.c`
- `src/00186.c`
- `src/00187.c`
- `src/00188.c`
- `src/00190.c`
- `src/00191.c`
- `src/00192.c`
- `src/00193.c`
- `src/00194.c`
- `src/00196.c`
- `src/00197.c`
- `src/00198.c`
- `src/00199.c`
- `src/00201.c`
- `src/00202.c`
- `src/00203.c`
- `src/00206.c`
- `src/00207.c`
- `src/00210.c`
- `src/00211.c`
- `src/00212.c`
- `src/00215.c`
- `src/00218.c`
- `src/00219.c`
- `src/00220.c`

Representative evidence called out by triage:

- `src/00025.c`: `strlen` call is followed by a bodyless emitted `strlen:`
  label and times out under qemu.
- `src/00056.c`, `src/00125.c`, `src/00179.c`, and `src/00220.c`: representative
  stdio/string/math/external rows with empty labels such as `ftrylockfile`,
  `__overflow`, `strlcpy`, `fmal`, or `printf`.

## In Scope

- Decide the supported RV64 behavior for external declarations and known
  library calls in generated assembly.
- Stop emitting executable empty bodies for unresolved external functions.
- For supported external calls, emit declarations/relocations so the platform
  linker and runtime can provide the implementation.
- For unsupported external calls or unavailable runtime surfaces, emit a clear
  unsupported diagnostic before producing a runnable binary.
- Add negative tests proving unsupported external calls fail explicitly rather
  than turning into bodyless stubs.

## Out Of Scope

- Implementing broad libc/libm/string functions inside the compiler as fake
  hand-written bodies.
- Making the entire c-testsuite pass by inventing named stubs for the observed
  filenames.
- Ordinary control/expression completion, local stack address materialization,
  global storage flow, and scalar storage lowering except where needed to keep
  external-call policy tests isolated.
- Reclassifying all external-call cases as unsupported without preserving
  supported runtime-link behavior where the target platform can supply it.

## Acceptance Criteria

- The backend no longer emits bodyless function labels that can be executed as
  accidental stubs for unresolved external calls.
- Supported external calls compile to assembly that links against the target
  runtime in the existing qemu probe shape, or unsupported ones fail before
  execution with an explicit diagnostic.
- Tests cover at least one string call, one stdio/libc-style call, one math
  call, and one user-declared unresolved external call.
- Targeted proof includes `src/00025.c`, `src/00056.c`, `src/00125.c`,
  `src/00179.c`, and `src/00220.c`, with results explained by the policy rather
  than by accidental fall-through behavior.
- Remaining runtime failures in this candidate bucket are documented as
  separate backend mechanisms if they are not caused by external empty stubs.

## Reviewer Reject Signals

- The implementation adds fake named bodies for c-testsuite-observed libc,
  libm, string, or user external symbols instead of defining a general policy.
- The patch simply marks all external-call tests unsupported without proving
  which calls should be unsupported and which should link to the target runtime.
- Runtime progress is claimed by expectation rewrites, skipped qemu execution,
  or weakened supported-path contracts.
- Bodyless executable labels remain in emitted assembly for unresolved external
  functions, even if one representative no longer reaches them.
- The route drifts into ordinary control/expression or local stack repair and
  claims external-call policy complete without proving empty-stub behavior is
  gone.
- The proof covers only one string testcase and does not exercise stdio/libc,
  math, and user-declared external shapes.
