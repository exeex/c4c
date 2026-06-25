# RV64I plus c4c EV64 roundtrip corpus.
#
# Contract boundary:
# - Supported extension target for this runbook is RV64I plus c4c EV64
#   `.insn.d`.
# - No M, A, F, D, C, vector-standard, CSR, system, or compressed extension
#   instructions are part of this corpus.
# - Canonical output is expected to preserve semantic instruction order, print
#   ABI register aliases consistently, and emit stable immediates/labels.
# - The intended proof route is:
#     input.s -> pass1.o -> pass1.s -> pass2.o -> pass2.s -> pass3.o
#   with pass1.s == pass2.s and pass2.o == pass3.o.
#
# Current implementation note:
# - This broad corpus intentionally exceeds the initial c4c-as/c4c-objdump
#   subset. Until later steps land, CTest must treat a fail-closed pass1
#   rejection as the expected state and must not claim full RV64I support.

.text
.globl c4c_rv64i_ev64_roundtrip_corpus
c4c_rv64i_ev64_roundtrip_corpus:
  # Upper-immediate and PC-relative forms.
  lui t0, 0x7ffff
  auipc t1, -0x80000

  # Register-immediate arithmetic/logical and edge immediates.
  addi t2, zero, -2048
  addi s0, t2, 2047
  slti s1, s0, -1
  sltiu s2, s0, 1
  xori s3, s0, -1
  ori s4, s0, 0x7f
  andi s5, s0, -128

  # Shift-immediate forms, including RV64 word shifts.
  slli s6, s5, 63
  srli s7, s6, 31
  srai s8, s6, 63
  addiw s9, s8, -2048
  slliw s10, s9, 31
  srliw s11, s10, 30
  sraiw a0, s10, 31

  # Register-register arithmetic/logical forms.
  add a1, a0, s1
  sub a2, a1, s2
  sll a3, a2, s3
  slt a4, a3, s4
  sltu a5, a4, s5
  xor a6, a5, s6
  srl a7, a6, s7
  sra t3, a7, s8
  or t4, t3, s9
  and t5, t4, s10

  # RV64 word register-register forms.
  addw t6, t5, a0
  subw ra, t6, a1
  sllw sp, ra, a2
  srlw gp, sp, a3
  sraw tp, gp, a4

  # Loads and stores with signed displacement edges.
  lb a0, -2048(sp)
  lh a1, -16(sp)
  lw a2, 0(sp)
  ld a3, 2040(sp)
  lbu a4, 15(sp)
  lhu a5, 30(sp)
  lwu a6, 60(sp)
  sb a0, -2048(sp)
  sh a1, -16(sp)
  sw a2, 0(sp)
  sd a3, 2040(sp)

  # Branches and labels.
.Lbranch_base:
  beq a0, a1, .Lbranch_equal
  bne a0, a1, .Lbranch_notequal
.Lbranch_equal:
  blt a0, a1, .Lbranch_signed
  bge a0, a1, .Lbranch_signed
.Lbranch_notequal:
  bltu a0, a1, .Lbranch_unsigned
  bgeu a0, a1, .Lbranch_unsigned
.Lbranch_signed:
  jal x1, .Ljump_target
.Lbranch_unsigned:
  jalr x0, 0(ra)
.Ljump_target:
  addi a0, a0, 1

  # c4c EV64 custom instruction remains in scope beside RV64I.
  .insn.d 10, 11, v6, v0, v2, v4, 3

  jalr zero, 0(ra)
