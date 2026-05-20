Status: Active
Source Idea Path: ideas/open/346_aarch64_direct_call_argument_formal_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Direct-Call ABI Publication Facts

# Current Packet

## Just Finished

Lifecycle switched from the backend-regex umbrella to the focused AArch64
direct-call argument/formal publication owner split from the committed Step 2
classification.

## Suggested Next

Execute `plan.md` Step 1: localize the current `00140`, `00159`, `00170`,
`00175`, and `00218` generated-code and prepared-state first bad facts across
caller-side argument publication and callee-side formal consumption.

## Watchouts

- This owner is direct-call ABI argument/formal publication only.
- Do not absorb pointer/null result publication, FP comparison
  materialization, aggregate/table memory corruption, libc/file/string
  residuals, semantic `lir_to_bir` admission, or timeout/output-storm buckets
  without fresh first-bad-fact evidence.
- Do not reopen closed ideas 309, 311, 336, 337, or 345 from counts or
  filename recurrence alone.
- Reject filename-specific, register-specific, expectation, unsupported,
  runner, timeout, proof-log, or CTest-registration changes.

## Proof

No implementation or validation command was run during this lifecycle-only
switch. The split used the existing backend-regex inventory log at
`/workspaces/c4c/test_after.log`: 354 selected, 330 passed, 24 failed, and 2
timed out.
