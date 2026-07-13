Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement core identity, ownership, order, and facade

# Current Packet

## Just Finished

- Step 3A implemented the frozen C++17 `Result<T, E>` / `Result<void, E>`
  carrier, full-field stable IDs and hashes, exact owner/generation/kind checked
  `SlotMap`, tombstone reuse with no-wrap retirement, and allocation-independent
  `IdOrder` traversal under the public `c4c::backend::bir` namespace.
- No semantic nodes, builders, facade, legacy compatibility, or persistent
  packet-local test source was added.

## Suggested Next

- Execute Step 3B: add bounded semantic ownership and read-only views, then the
  narrow public facade, without adding construction APIs.

## Watchouts

- `IdOrder` mutation is intentionally private; the packet-local proof accessed
  it only to prove duplicate/unknown errors before Step 3B/4 friends own it.
- Epoch allocation/publication exhaustion remains a builder/publication concern;
  Step 3A only establishes nonzero epoch validity and exact epoch resolution.
- Erasing a max-generation slot tombstones and permanently retires it instead
  of returning it to the free list.

## Proof

- Packet-local `/tmp` C++17 executable asserted Result success/error,
  cross-owner rejection, stale-ID rejection and generation change after reuse,
  wrong-`ValueKind` rejection, duplicate/unknown `IdOrder` errors, and semantic
  traversal order independent of reused-slot allocation — passed; temporary
  source and binary removed.
- Inline include-only translation unit via
  `c++ -std=c++17 -I/workspaces/c4c -x c++ -fsyntax-only` — passed.
- `rg -n 'c4c::bir' src/backend/bir/core` — no stale namespace matches.
- `git diff --check` — passed.
- The delegated packet explicitly excluded logs, so no `test_after.log` was
  written for this header-only packet-local proof.
