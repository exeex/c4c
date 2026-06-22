Status: Active
Source Idea Path: ideas/open/308_rv64_string_literals_and_extern_calls.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect String And Extern-Call Lowering Gaps

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and initialized execution state
for `plan.md` Step 1.

## Suggested Next

Execute Step 1: inspect RV64 prepared string constant data emission,
string-address materialization, and direct fixed-arity extern-call lowering for
the narrow representative `src/00025.c`, then record the smallest owner
surfaces to change.

## Watchouts

- Do not special-case `"hello"`, `strlen`, `.str0`, c-testsuite filenames, or
  exact source text.
- Do not broaden into aggregate, pointer, floating, scalar global storage,
  variadic calls, broad libc conformance, or full c-testsuite pass-count work.
- Do not weaken expectations, mark supported-path cases unsupported, or claim
  progress through classification-only changes.
- Keep scratch logs out of the repository root.

## Proof

Lifecycle-only activation. No build or code validation was required.
