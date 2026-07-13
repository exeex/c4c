Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement core identity, ownership, order, and facade

# Current Packet

## Just Finished

- Step 3B implemented the frozen bounded `Type`, value definitions, empty
  ordinary-instruction bootstrap, exact four terminators, private module and
  function ownership, and explicit function/block/instruction orders.
- Added borrowed read-only module/function/block views returning IDs, copies,
  or `Result`, with successor order derived exclusively from terminators, and
  restored the narrow `bir.hpp` facade without a legacy `Module` container.
- Follow-up review repair moved `ModuleData`, `FunctionData`, `BlockData`, and
  `InstData` into the explicit internal `bir::detail` namespace so transitive
  inclusion cannot present them as public facade types.

## Suggested Next

- Execute Step 4A: add builders and move-only `RawBir` publication ownership;
  the current build first needs the separately owned missing
  `bir/lir_to_bir.hpp` consumer seam addressed by its planned migration packet.

## Watchouts

- `ModuleView`, `FunctionView`, and `BlockView` are borrowed values; their
  lifetime must remain bounded by the future owning `RawBir`.
- An absent terminator is construction-only incomplete state. The future
  publication verifier must prevent such state from reaching a public view.
- The default build now passes the missing `bir.hpp` seam and first fails in
  `backend.hpp` because `bir/lir_to_bir.hpp` does not exist; backend consumer
  compatibility remains outside this packet and must not be solved by adding a
  legacy-shaped `bir::Module`.

## Proof

- Packet-local `/tmp` C++17 executable used temporary builder-private access to
  assert ordered view traversal, exact owner/epoch rejection, declaration and
  definition block visibility, value lookup, and terminator-derived successor
  order — passed; temporary source and binary removed.
- Negative facade probes confirmed unqualified public `ModuleData`,
  `FunctionData`, `BlockData`, and `InstData` do not compile, while semantic
  `Type`, `FunctionSignature`, and terminator values remain public — passed.
- Inline include-only translation unit via `c++ -std=c++17
  -I/workspaces/c4c/src -x c++ -fsyntax-only` — passed.
- `git diff --check`, stale-namespace search, and compile-database legacy-path
  search — passed with no matches.
- `cmake --build --preset default -j 2` — reached the next out-of-packet seam
  and failed because `src/backend/backend.hpp` includes missing
  `bir/lir_to_bir.hpp`.
- The delegated packet excluded logs, so no `test_after.log` was written.
