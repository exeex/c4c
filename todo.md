# Current Packet

Status: Active
Source Idea Path: ideas/open/802_project_wide_cpp20_host_toolchain_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish the durable host-toolchain contract

## Just Finished

- Plan Step 2 established one root CMake-3.20-compatible host authority with
  required C++20 and compiler extensions disabled for all in-tree production
  and native-test targets.
- Removed every in-scope target-local C++17 declaration while preserving the
  separate manual `gnu++20` language-under-test/workflow flags.
- Added configure-time probes for `__cplusplus >= 202002L`, designated
  initialization, `consteval`, and `std::span`, with a fatal diagnostic that
  names the selected compiler ID/version and each missing capability.
- Generated `build/compile_commands.json` contains 139 C++ commands; all 139
  use `-std=c++20`, with no host `gnu++17`, `gnu++20`, or `c++17` command.

## Suggested Next

- Execute plan Step 3 by publishing the durable host-toolchain contract and
  future-target consumption pattern without overstating compiler-family or
  version support beyond the capability probes and acceptance host evidence.

## Watchouts

- Keep c4cll language-under-test modes distinct from host compilation.
- Do not touch idea 732.
- The central authority is intentionally top-level rather than propagated by
  `c4c_target_profile`, because standalone tools and source-audit tests do not
  consume that library.
- Only clang 22.1.7 plus libc++ 220107 is evidenced by the current acceptance
  host; Step 3 must not turn that observation into unsupported portability
  claims for other compiler families or versions.
- Preserve the four manual `-std=gnu++20` language-under-test/workflow flags;
  they are outside the host-target migration.

## Proof

- `cmake --preset default -DENABLE_C4C_BACKEND=ON && cmake --build --preset default`
  succeeded with all four C++20 capability probes passing.
- Compile-command inspection found `-std=c++20` on all 139 generated C++ host
  commands and no old or extension-mode host standard flag.
- Focused native proof passed 6/6 tests: `backend_bir_node_kind_schema`,
  `backend_bir_pipeline_identity`, `frontend_lexer_tests`,
  `frontend_parser_tests`, `frontend_hir_lookup_tests`, and
  `frontend_cxx_preprocessor_tests`.
- Per the delegated Step 2 proof contract, canonical `test_after.log` remains
  supervisor-owned and was not rewritten by this packet.
