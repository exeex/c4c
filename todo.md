# Current Packet

Status: Active
Source Idea Path: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify native pointer/object/lifetime authority

## Just Finished

-  753 Step 2 repaired direct-local aggregate-zero `memset` admission: only
  a strictly positive known aggregate size receives native memory/VA
  authority; a zero-sized aggregate remains compatibility-only. Added the
  adjacent zero-sized-array lowering/verifier coverage. The five reproduced
  baseline regressions and the focused authority guard pass, and the fresh
  full candidate baseline is 3037/3037.

## Suggested Next

- Supervisor: review and commit the completed 753 Step 2 baseline-repair
  slice, then select the next bounded Step 2 producer packet.

## Watchouts

- Native aggregate-zero local `memset` authority requires a positive known
  size. memcpy remains historical selected-only; builtin/indirect VA, va_copy,
  and va_arg (including aggregate memcpy-like vaarg moves) remain
  compatibility-only because they cannot yet retain the complete tuple. Do not
  derive facts from display text or widen the historical memcpy descriptor.

## Proof

- Baseline-repair proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  passed 5/5. The five reproduced regressions passed individually. The same
  full-suite command used for `test_baseline.new.log` produced a fresh
  3037/3037 candidate. `git diff --check` remains required before commit.
