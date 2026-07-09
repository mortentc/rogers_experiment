int prog(void *ctx, int mem_size) {
  short *mem = (short *)ctx;

  if (*mem == 1)
    *mem = 2;

  return 0;
}