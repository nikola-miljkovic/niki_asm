            .public proizvod ;
            .extern pomnozi ;asd
.data
prviBroj:   .word 5        ; komentar
            .skip 20
drugiBroj:  .long -20       ; komentar2
output:     .long 8192      ; memorija za output
.text
main:       ldc r5, -824312
            ldc r6, -824312
            adds r0, prviBroj
            adds r1, drugiBroj
            add  r0, r1 ; saberi prviBroj + drugiBroj
            call r3, pomnozi    ; r3 je nula, upisace u r0
            add r4, output
            out r0, r4          ; ispisi na output
.bss
proizvod:   .long 0             ; ovde ce upisati
.end
