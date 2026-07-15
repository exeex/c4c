# Current Packet

Status: Active
Source Idea Path: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify native pointer/object/lifetime authority

## Just Finished

-  753 Step 2 first admitted subpacket completed the native-only memory/VA
  descriptor and verifier boundary for memcpy, memset, va_start, va_end,
  va_copy, and va_arg. Native local aggregate-zero memset emission now
  publishes its checked destination, i8 byte, and i64 positive size facts.
  The verifier rejects missing, unselected, foreign, type-mismatched,
  size-mismatched, and dead authority.

## Suggested Next

- Continue 753 Step 2 only: admit an additional representative memory or
  va-list producer form only if it retains the complete native tuple. Keep VA
  routes compatibility-only until then; do not begin Raw-BIR receiver work.

## Watchouts

- The admitted producer is only local aggregate-zero memset where the emitter
  retains the full 752 tuple. memcpy remains historical selected-only; builtin
  memcpy, va_start, va_end, va_copy, and va_arg (including aggregate
  memcpy-like vaarg moves) remain compatibility-only because they cannot yet
  retain the complete tuple. Do not derive it from display text or widen the
  historical memcpy descriptor.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  passed: 5/5 backend tests. `git diff --check` passed. Proof log:
  `test_after.log`.
