Status: Active
Source Idea Path: ideas/open/09_bir-call-signature-and-abi-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit helper boundaries and wire the new file

# Current Packet

## Just Finished

Activation created the canonical runbook and execution-state skeleton for
Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: audit the candidate helper boundaries in
`calling.cpp`, create `src/backend/bir/lir_to_bir/call_abi.cpp`, wire the
hard-coded backend notes test source list, and prove the initial build target.

## Watchouts

- This is a behavior-preserving extraction; do not rewrite expectations.
- Do not add new headers.
- Keep `lower_call_inst` and `lower_runtime_intrinsic_inst` in `calling.cpp`.
- `src/codegen/CMakeLists.txt` uses recursive source discovery, but
  `tests/backend/CMakeLists.txt` currently lists `lir_to_bir` implementation
  sources explicitly for `backend_lir_to_bir_notes_test`.

## Proof

Not run during lifecycle activation; no implementation files were changed.
