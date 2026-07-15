# Current Packet

Status: Active
Source Idea Path: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify native pointer/object/lifetime authority

## Just Finished

-  753 Step 2 VA representative subpacket admitted direct-local `va_start`
  and `va_end` only when the HIR DeclRef and current-function local map retain
  the full 752 pointer/object/owner/type/live tuple. The existing verifier
  rejects missing, foreign, type-mismatched, or dead selected VA authority;
  no va-list lifecycle state was inferred.

## Suggested Next

- Continue 753 Step 2 only: admit an additional representative memory or
  va-list producer form only if it retains the complete native tuple. Keep
  builtin/indirect VA and all remaining routes compatibility-only until then;
  do not begin Raw-BIR receiver work.

## Watchouts

- Admitted producers are local aggregate-zero memset and direct-local
  va_start/va_end. memcpy remains historical selected-only; builtin/indirect
  VA, va_copy, and va_arg (including aggregate memcpy-like vaarg moves) remain
  compatibility-only because they cannot yet retain the complete tuple. Do not
  derive it from display text or widen the historical memcpy descriptor.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  passed: 5/5 backend tests. `git diff --check` passed. Proof log:
  `test_after.log`.
