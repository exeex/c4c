.text
.globl main
.type main, %function
main:
  bl helper
  mov w0, #0
  ret
