void f(unsigned long* dst, unsigned long count, unsigned long value) {
    __asm__ __volatile__("cld\n\t"
                         "rep stosq\n\t"
                         : "+c"(count), "+D"(dst), "=m"(dst)
                         : "a"(value)
                         : "cc");
}
