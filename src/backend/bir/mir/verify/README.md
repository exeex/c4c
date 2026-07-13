# MIR Verification

Status: scaffold.

Verification profiles correspond to constructed, selected, call-lowered,
allocated, frame-laid-out, and final MIR. Each profile rejects facts that are
missing too late or realized too early.

Checks include identity and ownership, CFG, operand classes, constraints,
virtual/physical register legality, call sequence agreement, spill/frame slot
validity, stack alignment, prologue/epilogue balance, target opcode legality,
and absence of unresolved parallel copies before emission.
