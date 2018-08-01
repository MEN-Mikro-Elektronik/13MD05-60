descgen -r test_gen_desc.dsc
copy test_gen_desc.c + desc_test.c  test_desc.c
set CPU=PPC603
make test_desc.o