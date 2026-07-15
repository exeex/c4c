# Current Packet

Status: Active
Source Idea Path: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify native pointer/object/lifetime authority

## Just Finished

-  753 Step 2 admitted native aggregate-zero `memset` authority (commit
  `b334a1b29`) and direct-local `va_start`/`va_end` authority (commit
  `8f6f4f9c9`). Both passed the five-test backend authority guard. The
  hook-required full baseline was then rejected: prior `test_baseline.log`
  passed 3037/3037; `test_baseline.new.log` at `8f6f4f9c9` passed 3032/3037
  and newly failed five non-guard cases.

## Suggested Next

- Repair the Step 2 authority publication/regression before another commit.
  Reproduce and diagnose the five new baseline failures
  (`clang_c_external_C_C23_n2900_n3011_2_c`,
  `cpp_positive_sema_constrained_template_method_call_frontend_cpp`,
  `llvm_gcc_c_torture_src_strcpy_2_c`,
  `llvm_gcc_c_torture_src_zero_struct_1_c`, and
  `llvm_gcc_c_torture_src_zero_struct_2_c`) by native semantics, not
  testcase-shaped exceptions. Retain only authority publication that is valid
  for its semantic producer family; keep the unsupported routes
  compatibility-only. Before another implementation commit, rebuild, rerun
  the focused authority guard, and require a fresh full baseline with no new
  failures relative to the accepted 3037/3037 baseline.

## Watchouts

- Admitted producers are local aggregate-zero memset and direct-local
  va_start/va_end. memcpy remains historical selected-only; builtin/indirect
  VA, va_copy, and va_arg (including aggregate memcpy-like vaarg moves) remain
  compatibility-only because they cannot yet retain the complete tuple. Do not
  derive it from display text or widen the historical memcpy descriptor.
- The five baseline failures are regression evidence, not permission to add
  named-case matching, expectation downgrades, rendered-text recovery, or
  weaker verifier contracts. Step 2 remains incomplete until the delta is
  eliminated or an evidence-backed, source-compatible repair is accepted.

## Proof

- Accepted narrow proof for both commits:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  passed 5/5; `git diff --check` passed. Repaired acceptance additionally
  requires the same fresh full-suite baseline command used for
  `test_baseline.new.log`, with 3037/3037 passing or no new failures versus
  the accepted `test_baseline.log` (3037/3037).
