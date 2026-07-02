Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Local-Memory Producer Boundary

# Current Packet

## Just Finished

Lifecycle activated
`ideas/open/557_bir_local_memory_semantic_producer_admission.md` after closing
`ideas/open/545_bir_semantic_producer_admission_reconstruction.md`.

## Suggested Next

Begin Step 1 from `plan.md`: inspect the current local-memory evidence rows,
`src/backend/bir/lir_to_bir.cpp`, and `src/backend/bir/lir_to_bir/memory/` to
identify the first missing or malformed BIR semantic facts for load, GEP,
store, scalar/local-memory, and alloca.

## Watchouts

- Local-memory is the active lane because it has the largest verified exact
  semantic row count: `264`.
- Keep call metadata, runtime/intrinsic, scalar/signature/control, and
  bootstrap/global data-shape work in their separate source ideas.
- Do not infer missing address, provenance, or memory facts in RV64/MIR.
- Do not weaken semantic admission checks, expectations, unsupported markers,
  allowlists, or runtime comparison behavior.
- Representative local-memory proof seeds are `src/20000314-1.c`,
  `src/20000717-4.c`, `src/20001026-1.c`, `src/20000519-1.c`, and
  `src/20050604-1.c`.

## Proof

Lifecycle-only activation. Existing close-time regression logs were checked
with:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_before.log \
  --after test_after.log \
  --allow-non-decreasing-passed
```

Result: pass, with `346` passed before and after, `0` failed, and no new
failures. No implementation files, tests, expectation files, unsupported
markers, allowlists, or runtime comparison behavior were edited.
