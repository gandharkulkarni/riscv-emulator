.global fib_rec_s

# fibrec - compute the nth fibonacci number

# a0 - int n

fib_rec_s:
    addi sp, sp, -32 # Must be a multiple 16
    sd ra, (sp)      # Preserve ra on stack

    
    li t0, 2
    blt a0, t0, return
    
    recurse:
    li t1, 1
    sd a0, 8(sp)    # preserve a0 before n-1 operation
    sub a0, a0, t1  
    call fib_rec_s
    mv t2, a0
    sd t2, 16(sp)   # preserve t2 before 2nd recursive call
    
    ld a0, 8(sp)    # restore a0 before n-2 operation
    sub a0, a0, t1  # n-1
    sub a0, a0, t1  # n-1
    call fib_rec_s

    ld t2, 16(sp)   # restore t2 after recursive call
    add a0, a0, t2

    return:
    ld ra, (sp)      # Restore ra from stack
    addi sp, sp, 32  # Deallocate stack
    ret

