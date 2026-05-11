# Whole-Codebase String Authority Final Audit

Status: Open
Created: 2026-05-11

Depends On:
- `ideas/closed/166_hir_rendered_registry_mirror_retirement_audit.md`

Parent Ideas:
- `ideas/closed/158_sema_validate_string_authority_audit.md`
- `ideas/closed/159_sema_consteval_domain_table_authority.md`
- `ideas/closed/160_sema_canonical_symbol_template_key_authority.md`
- `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- `ideas/closed/162_link_name_id_backend_symbol_authority.md`

## Goal

Perform a final whole-codebase audit for remaining string-keyed semantic
authority after the parser, sema, HIR, LinkNameId, BIR, and rendered-registry
cleanup series.

This idea is an audit and classification pass first. It should not assume that
every remaining `std::string` or `unordered_map<string, ...>` is wrong. It
should identify which string paths are still semantic authority, which are
explicit compatibility bridges, and which are valid output, diagnostic, or
route-local names.

## Why This Idea Exists

Ideas 153-166 remove or fence the known high-risk string authority regions.
After 166 closes, the next risk is hidden leftovers: small lookup helpers,
legacy overloads, rendered-name fallback APIs, tests, or backend utilities that
still use raw strings as identity despite structured domains existing nearby.

A final audit gives us a map before we start deleting compatibility paths. It
also prevents the project from drifting into whack-a-mole cleanup based only
on grep hits.

## In Scope

- Search the codebase for string-keyed semantic patterns, including:
  `unordered_map<std::string, ...>`, `map<std::string, ...>`,
  `find(name)`, `lookup(name)`, `*_by_name`, `*_name_map`,
  `mangled_name`, `source_name`, `qualified_name`, and raw rendered fallback
  helpers.
- Classify every meaningful hit into one of:
  semantic authority, compatibility bridge, display/output, diagnostic/debug,
  route-local identity, generated temporary name, or false positive.
- Verify that compatibility bridges are fenced by an explicit no-metadata or
  invalid-id boundary.
- Verify that complete structured misses fail closed for previously covered
  parser/sema/HIR/LIR/BIR/backend domains.
- Identify remaining semantic-authority strings that need follow-up ideas.
- Produce a durable audit artifact or idea summary listing retained bridges
  and follow-up recommendations.

## Out Of Scope

- Large code rewrites during the audit pass.
- Removing compatibility bridges before they are classified.
- Replacing route-local names such as SSA temps, labels, stack slots,
  registers, or string-pool labels.
- Treating output/dump/diagnostic strings as bugs just because they are
  strings.

## Acceptance Criteria

- A whole-codebase string-authority inventory exists and is easy to review.
- Remaining semantic-authority string paths are either fixed if tiny or listed
  as concrete follow-up work.
- Retained string paths are classified with owner, domain, and removal
  condition when applicable.
- The audit distinguishes source identity, semantic domain identity,
  link-visible identity, route-local identity, and display text.
- No broad refactor or testcase weakening is hidden inside the audit.

## Reviewer Reject Signals

- The audit treats all strings as equally bad.
- A known semantic string lookup is reclassified as display without evidence.
- A complete structured miss still falls back to rendered string lookup in a
  covered domain.
- The work silently expands into unrelated implementation without a follow-up
  idea.
