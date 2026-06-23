# RV64 Local Stack Slot And Address Materialization

## Goal

Repair RV64 lowering for local stack slots, address-taken locals, arrays,
aggregates, pointer values, and function-pointer storage so local effective
addresses and loaded values are materialized before use.

## Why This Exists

The RV64 c-testsuite runtime triage classified 21 runtime failures as
`stack_pointer_local_slot_materialization`. This is the second-ranked repair
family in `todo.md` because it has nearly the same observed impact as ordinary
control/expression completion, but it carries higher aliasing and ABI risk.

Step 4 evidence in
`build/rv64_c_testsuite_probe_latest/triage_step4/summary.md` describes this
family as local pointer, array, aggregate, function-pointer, or address-taken
stack object lowering that loses or fails to materialize the effective
address/value path before use.

## Owned Feature Family

RV64 backend lowering for stack-frame local object allocation, local slot
address publication, loads and stores through local effective addresses, array
element addressing, aggregate-local access, pointer-to-local flow, and
function-pointer local storage/use.

## Candidate Cases

Primary runtime candidates from the Step 4 classification:

- `src/00005.c`
- `src/00018.c`
- `src/00019.c`
- `src/00026.c`
- `src/00032.c`
- `src/00037.c`
- `src/00039.c`
- `src/00043.c`
- `src/00046.c`
- `src/00058.c`
- `src/00072.c`
- `src/00073.c`
- `src/00077.c`
- `src/00078.c`
- `src/00087.c`
- `src/00124.c`
- `src/00130.c`
- `src/00137.c`
- `src/00138.c`
- `src/00140.c`
- `src/00144.c`

Representative evidence called out by triage:

- `src/00005.c`: local pointer path includes suspicious stack loads such as
  `ld s2, 8(sp)` and `lw s2, 0(sp)`.
- `src/00032.c`: array slots are stored, but pointer/indexed use is not
  completed.
- `src/00130.c`: array and element addresses are stored, but indexed access
  checks are not completed.

## In Scope

- Audit prepared BIR to RV64 lowering for local object slots and address-taken
  values.
- Materialize local effective addresses and loaded values with correct RV64
  register width, stack offsets, and lifetime assumptions.
- Cover arrays, pointer-to-local, aggregate-local, and function-pointer local
  flows with focused tests.
- Preserve the distinction between local stack/address issues and unrelated
  global storage or external-call policy buckets.

## Out Of Scope

- Global object storage/address flow beyond comparisons needed to isolate local
  behavior.
- External libc/libm/string call policy and bodyless-stub emission.
- Generic scalar width-storage cleanup unless required for local address proof.
- Rewriting C-test expectations or broadening unsupported diagnostics to hide
  local address failures.

## Acceptance Criteria

- Representative local stack/address cases compile, link, and run under qemu
  without losing local effective addresses or dereferencing unmaterialized
  values.
- New or updated tests prove local pointer, array element, address-taken local,
  aggregate-local, and function-pointer local behavior at the backend level.
- The implementation generalizes through stack slot/address lowering APIs or
  RV64 emission rules rather than matching candidate filenames.
- Targeted proof includes at least `src/00005.c`, `src/00032.c`, `src/00072.c`,
  and `src/00130.c`, plus nearby bucket candidates.
- Remaining failures in the candidate set are reclassified with concrete
  emitted-code evidence rather than left as ambiguous runtime crashes.

## Reviewer Reject Signals

- The patch special-cases offsets, source filenames, local variable names, or
  known c-testsuite shapes instead of repairing local address materialization.
- The proof relies on expectation rewrites, unsupported downgrades, or skipped
  runtime checks.
- Local stack tests pass only because a generic fall-through or epilogue was
  added while local effective-address values remain missing or wrong.
- The route claims this idea complete using only scalar non-pointer cases and
  does not exercise arrays, address-taken locals, and pointer-to-local flows.
- The implementation conflates global object materialization with local stack
  slot lowering and leaves the local candidate failures unchanged.
- A helper rename or abstraction preserves the same bad `sp` offset/load/store
  behavior behind a new interface name.
