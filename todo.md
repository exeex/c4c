Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement core identity, ownership, order, and facade

# Current Packet

## Just Finished

- Step 3 repair moved `Slot`, `SlotMap`, `IdOrder`, `StorageError`, `OrderError`,
  and ID-access helpers under `bir::detail`; only `ResolveError` remains public
  for view result signatures.
- Storage allocation/mutable resolution/erase and semantic-order mutation are
  private and friend only the frozen module/function builders and foundation
  verifier. Detail-owned const lookup remains available to read-only views.
- Unqualified public `SlotMap`, `IdOrder`, and ownership data types are absent
  from the facade; runtime identity, reuse, and view semantics are unchanged.

## Suggested Next

- Execute Step 4A against the repaired private mutation seam: add builders and
  move-only `RawBir` publication ownership without exposing detail containers.

## Watchouts

- `ModuleView`, `FunctionView`, and `BlockView` are borrowed values; their
  lifetime must remain bounded by the future owning `RawBir`.
- Builder implementations must use their narrow friendship; they must not make
  detail storage/order mutation public or re-export detail aliases.
- An absent terminator remains construction-only incomplete state; the future
  publication verifier must prevent it from reaching a public view.

## Proof

- Packet-local Step 3A C++17 runtime assertions for Result, exact owner/kind,
  generation reuse, stale IDs, and allocation-independent order — passed.
- Packet-local Step 3B C++17 runtime assertions for ordered views, owner/epoch
  rejection, declaration/definition visibility, value lookup, and
  terminator-derived successors — passed; temporary sources/binaries removed.
- Negative facade probes confirmed unqualified public `SlotMap`, `IdOrder`,
  and `ModuleData` do not compile — passed.
- Inline include-only translation unit via `c++ -std=c++17
  -I/workspaces/c4c/src -x c++ -fsyntax-only` — passed.
- `git diff --check` plus stale `c4c::bir` and legacy-path scans — passed with
  no matches.
- The delegated packet excluded logs, so no `test_after.log` was written.
