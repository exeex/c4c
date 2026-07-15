# Current Packet

Status: Active
Source Idea Path: ideas/open/802_project_wide_cpp20_host_toolchain_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Converge the NodeKind C++20 authoring surface

## Just Finished

- Plan Step 3 published `docs/host_toolchain.md` as the durable contract for
  strict non-extension C++20, the root CMake authority, configure-time
  capability probes, fail-fast behavior, and future in-tree target authors.
- Recorded the verified Clang 22.1.7, libc++ 220107, arm64 Darwin acceptance
  host without claiming untested compiler families or versions.
- Linked the contract from both the README build instructions and the
  getting-started document, while keeping third-party boundaries and c4cll
  language-under-test modes explicitly separate.

## Suggested Next

- Execute plan Step 4 by converging the NodeKind registry authoring surface on
  the approved, named, locally validated C++20 form without changing its
  semantics or pass-facing query API.

## Watchouts

- Do not touch idea 732.
- Preserve the existing NodeKind tag algebra, stage vocabulary, one-registry
  authority, and public compile-time/runtime helper behavior.
- Keep the C++20 authoring syntax agent-readable; avoid turning the registry
  into a template metaprogramming DSL or introducing unrelated modernization.

## Proof

- `git diff --check` passed.
- Focused search of `README.md`, `docs/getting_started.md`, and
  `docs/host_toolchain.md` found no stale host C++17 build guidance.
- This documentation-only packet required no build and did not rewrite the
  supervisor-owned canonical `test_after.log`.
