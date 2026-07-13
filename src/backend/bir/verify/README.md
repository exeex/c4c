# BIR Verification Contract

Status: scaffold over an existing partial verifier.

Required profiles are Raw, Canonical, and PreparedInput. Verification covers ID
ownership/generation, order membership, types and opcode roles, exact def-use,
dominance, terminators, CFG/phi agreement, call signatures, stage legality, and
the absence of legacy/prepared/MIR authority in canonical storage.

Debug/test mode verifies after every mutating pass. Publication of `RawBir`,
`CanonicalBir`, and `PreparedBir` always verifies. Diagnostics are structured by
rule and semantic ID; rendered strings are never used to make decisions.

Legacy coverage includes `prealloc/prepared_contract_verifier.*`; every legacy
agreement check must become a named stage invariant or be deleted as duplicate
authority.
