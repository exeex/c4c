# Current Packet

Status: Active
Source Idea Path: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify native pointer/object/lifetime authority

## Just Finished

-  753 Step 2 admitted `BuiltinId::VaCopy` to native memory/VA authority only
  when both operands are direct current-function locals carrying the existing
  pointer/object/owner/liveness tuple. Other forms retain the compatibility
  path. Added direct-local lowering coverage plus incomplete, foreign, and
  dead two-sided authority verifier coverage.

## Suggested Next

- Supervisor: review and commit the completed 753 Step 2 `va_copy` authority
  slice, then select the next bounded Step 2 producer packet.

## Watchouts

- Native aggregate-zero local `memset` authority requires a positive known
  size. memcpy remains historical selected-only; builtin/indirect VA and
  va_arg (including aggregate memcpy-like vaarg moves) remain
  compatibility-only because they cannot yet retain the complete tuple.
  `va_copy` requires both direct-local operands; a one-sided or non-direct
  form must stay compatibility-only. Do not derive facts from display text or
  widen the historical memcpy descriptor.

## Proof

- `va_copy` authority proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  passed 5/5; `test_after.log` is the proof log. `git diff --check` remains
  required before commit.
