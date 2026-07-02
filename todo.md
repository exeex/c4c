Status: Active
Source Idea Path: ideas/open/425_scalar_fpr_salvage_from_try_gcc_torture.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Follow-Up Idea Boundaries

# Current Packet

## Just Finished

Step 3 - Define Follow-Up Idea Boundaries converted the accepted Step 2
candidate into one proposed follow-up boundary and recorded reject signals
before any source idea files are created.

Proposed follow-up boundary: residual scalar F32/F64 cast object lowering.

- Owning layer: RV64 prepared object lowering for scalar FPR/GPR conversion
  casts, with prepared/BIR producer facts treated as inputs rather than
  inferred by the object emitter.
- Evidence and bucket impact: the fresh 2026-07-01 scan has exactly `3`
  `unsupported_floating_cast` rows in
  `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`:
  `src/cvt-1.c`, `src/920618-1.c`, and `src/pr66233.c`. Their logs report
  that the RV64 object route currently supports only prepared FPR width casts
  and I32/I64-to-F32/F64 integer-to-floating casts.
- Branch evidence to mine, not cherry-pick: `9d0f64883` floating-cast producer
  facts, `b0700c2c3` prepared FP-to-integer casts, `88076c4ec` F32 integer
  casts, `18c41c9c3` int-to-F32 materialization, `92d77cf2c` int-to-F64
  materialization, `8bbaf8eb7` FPR width casts, and `f95ec37b9` F64-to-F32
  trunc immediates.

In scope for the follow-up:

- Identify the concrete cast opcodes and source/result scalar types for the
  three fresh rows before implementation.
- Add semantic RV64 object lowering only for supported scalar F32/F64 cast
  directions whose prepared facts are explicit: cast opcode, source type,
  result type, source/destination bank, materializable immediate or coherent
  value home, and no F128/helper dependency.
- Preserve the existing accepted paths for prepared FPR width casts and
  I32/I64-to-F32/F64 casts.
- Add focused backend object-emission tests that construct representative
  prepared cast fixtures directly, plus a fail-closed test for an unsupported
  scalar FP cast direction.
- Keep diagnostics owner-specific when prepared facts are missing,
  contradictory, or outside the supported scalar F32/F64 contract.

Out of scope for the follow-up:

- F128/long-double casts, helper calls, helper ABI, F128 lane pairs, F128
  local-memory work, and `conversion.c` completion.
- Aggregate/byval, stack-frame, parameter-home, or local-memory repairs.
- Scalar FPR binary arithmetic, branch guard lowering, select/materialization,
  local-store/reload publication, same-module scalar FPR calls, and return
  move bundles unless a future scan proves one of them as the first owner.
- Direct cherry-picks from `try_gcc_torture`, expectation rewrites, and
  filename/testcase-shaped matching.

Representative proof for the follow-up:

- Focused representative external probes:
  `tests/c/external/gcc_torture/src/cvt-1.c`,
  `tests/c/external/gcc_torture/src/920618-1.c`, and
  `tests/c/external/gcc_torture/src/pr66233.c`.
- Focused backend unit proof: run the RV64 object-emission test binary or the
  matching CTest entry after adding prepared cast fixtures.
- Default CTest expectation: `ctest --test-dir build -j --output-on-failure -R
  '^backend_'` for acceptance, because the implementation surface is RV64
  backend object lowering and cast diagnostics.
- Full gcc_torture scan is evidence, not a default gate; supervisor may request
  it only for milestone confidence.

Dependencies and preconditions:

- Step 3 follow-up idea creation should first preserve the exact row evidence
  from `build/rv64_gcc_c_torture_backend/src_cvt-1.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_920618-1.c/case.log`, and
  `build/rv64_gcc_c_torture_backend/src_pr66233.c/case.log`.
- If row inspection shows the first owner is missing prepared cast facts rather
  than RV64 lowering, split a producer/prepared fact idea instead of widening
  object emission.
- If any row depends on F128/helper ABI, aggregate/byval, stack-frame, or
  local-memory authority, remove it from this follow-up and record quarantine.

Reviewer reject signals:

- The patch uses `conversion.c`, a filename, or one of the three row names as
  the main proof instead of focused semantic prepared-cast tests.
- The patch imports broad `try_gcc_torture` code or direct cherry-picked
  behavior rather than rewriting a small current-mainline boundary.
- The patch changes expectations, allowlists, or unsupported contracts to claim
  progress.
- The patch admits F128/helper ABI/local-memory, aggregate/byval, stack-frame,
  scalar call/return, branch/select, or local-store/reload behavior under the
  scalar cast follow-up.
- The patch infers missing bank/type/home facts in RV64 object emission instead
  of requiring explicit prepared facts or failing closed.
- The proof only shows a named gcc_torture row and does not cover nearby
  supported/unsupported scalar cast directions.

Why other candidates are supporting context only:

- The older scalar residual rows `src/ieee/930529-1.c` and
  `src/ieee/pr72824.c` remain useful context but are not follow-up boundaries:
  `930529-1` mixes a `double` division with heavy select/local-byte checks, and
  `pr72824` mixes `float` local storage, `fptrunc`, `llvm.copysign.f32`, and a
  floating branch guard. They are too composite and too old-row-specific to
  drive the first scalar-FPR salvage idea.
- F128/helper ABI/local-memory, aggregate/byval, stack-frame, and
  `conversion.c`-only work are not follow-up ideas from this route because
  they either belong to different owners or depend on low-value branch history
  that Step 2 explicitly quarantined.

## Suggested Next

Delegate Step 4 to publish the salvage handoff and create a single ordered
follow-up idea for residual scalar F32/F64 cast object lowering, unless the
plan owner decides the boundary should be split first by prepared fact
availability.

## Watchouts

Do not create source idea files from this executor packet. Step 4/plan-owner
should create at most one follow-up idea from the accepted boundary unless row
inspection proves the three fresh rows have different first owners. Keep the
older scalar residual rows as context, not acceptance criteria.

## Proof

Docs/classification-only step. No build or CTest was required.

- `git diff --check -- todo.md`
