PROG = project03
OBJS = project03.o rv_emu.o bits.o cache.o verbose.o\
	   quadratic_c.o quadratic_s.o\
	   midpoint_c.o midpoint_s.o\
	   max3_c.o max3_s.o\
	   to_upper_c.o to_upper_s.o\
       get_bitseq_c.o get_bitseq_s.o\
       get_bitseq_signed_c.o get_bitseq_signed_s.o\
	   fib_rec_c.o fib_rec_s.o\
	   swap_c.o swap_s.o\
	   sort_c.o sort_s.o\
	   add2_c.o add2_s.o

HEADERS = project03.h rv_emu.h bits.h

%.o: %.c $(HEADERS)
	gcc -g -c -o $@ $<

%.o: %.s
	as -g -o $@ $<

$(PROG): $(OBJS)
	gcc -g -o $@ $^

clean:
	rm -rf $(PROG) $(OBJS)
