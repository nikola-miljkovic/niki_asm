            .public pomnozi ;
            .extern proizvod ;asd
.data
mnozilac:   .long 24
.text
pomnozi:    mul r0, r1
            mul r0, mnozilac
            mov pc, lr  ; izadji
.end
