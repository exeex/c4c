Status: Active
Source Idea Path: ideas/open/308_rv64_string_literals_and_extern_calls.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect String And Extern-Call Lowering Gaps

# Current Packet

## Just Finished

Completed `plan.md` Step 1, "Inspect String And Extern-Call Lowering Gaps":
captured the `src/00025.c` prepared-BIR and RV64 asm-output facts, then recorded
the smallest owner surfaces in `build/rv64_string_call_inspection.md`.

## Suggested Next

Implement the next narrow packet by teaching the RV64 prepared module emitter
to emit addressable string/byte constant storage for prepared string globals,
then rerun the same representative to expose the next function-emission blocker.

## Watchouts

- Do not special-case `"hello"`, `strlen`, `.str0`, c-testsuite filenames, or
  exact source text.
- The prepared call plan for `strlen` is already
  `direct_extern_fixed_arity`; the first blocker is RV64 string storage
  emission, followed by RV64 handling for `StringConstant` address
  materialization in pointer local stores.
- Do not make string globals look like no-storage externs; the bytes need
  emitted addressable storage.
- Do not broaden into aggregate, pointer, floating, scalar global storage,
  variadic calls, broad libc conformance, or full c-testsuite pass-count work.
- Do not weaken expectations, mark supported-path cases unsupported, or claim
  progress through classification-only changes.
- Keep scratch logs out of the repository root.

## Proof

Ran the supervisor-selected proof exactly; `test_after.log` is preserved. The
build succeeded and the command captured the prepared-BIR plus current RV64
diagnostic, but the proof exited nonzero because the final `rg` names
`build/rv64_string_call_inspection/src_00025.s` and the current failing RV64
asm command does not produce that file.
