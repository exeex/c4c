# Current Packet

Status: Active
Source Idea Path: ideas/open/367_semantic_bir_indirect_local_memory_lvalue_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Indirect Local-Memory Rejection

## Just Finished

Lifecycle switch from umbrella idea 295 to focused idea 367 is complete.

## Suggested Next

Executor should start Step 1 by localizing the semantic `lir_to_bir`
local-memory rejection shared by `c_testsuite_aarch64_backend_src_00005_c`
and `c_testsuite_aarch64_backend_src_00217_c`, then record the first bad
branch and focused proof command results here.

## Watchouts

- Keep AArch64/runtime/runner/expectation changes out of scope.
- Do not treat the current local backend-route snippet drift as progress for
  this semantic admission owner.
- Preserve the completed `00173` pointer-derived load/publication path.

## Proof

No code proof was run for this lifecycle-only switch. The split is based on
the committed Step 2 classification in `a4093586e`.
