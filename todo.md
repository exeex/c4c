# LIR To BIR Local Memory Admission Todo

Status: Active
Source Idea Path: ideas/open/297_lir_to_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Isolate The Admission Failure

# Current Packet

## Just Finished

Lifecycle switch completed from umbrella idea 295 to focused idea 297.
Implementation is now attached to
`ideas/open/297_lir_to_bir_local_memory_admission.md`.

## Suggested Next

Execute Step 1: inspect representative local-memory GEP, store, and load
failures to identify the exact `lir_to_bir` rejection path and the semantic
local-memory form the implementation must admit.

## Watchouts

- Focused GEP local-memory cases: `00143`, `00157`, `00176`, `00181`,
  `00182`, `00185`, `00195`, `00205`, `00209`.
- Store/load boundary checks: `00046`, `00140`, `00216`, `00218`.
- `00204` is a separate bootstrap global aggregate/array semantics gate; do
  not fold it into this owner without evidence.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, runner behavior, `test_after.log`, or
  `test_before.log`.
- Do not claim progress through filename matching or named-case shortcuts.

## Proof

Lifecycle-only switch. No build, compile, or CTest proof was run.
