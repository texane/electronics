#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


/* memory mapped file */

typedef struct mapped_file
{
  uint8_t* base;
  size_t len;
} mapped_file_t;

#define MAPPED_FILE_INITIALIZER { NULL, 0 }

static int map_file(mapped_file_t* mf, const char* path)
{
  int error = -1;
  struct stat st;

  const int fd = open(path, O_RDONLY);
  if (fd == -1) return -1;

  if (fstat(fd, &st) == -1) goto on_error; 

  mf->base = (uint8_t*)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (mf->base == MAP_FAILED) goot on_error;

  mf->len = st.st_size;

  /* success */
  error = 0;

 on_error:
  close(fd);

  return error;
}

static void unmap_file(mapped_file_t * mf)
{
  munmap((void*)mf->base, mf->len);
  mf->base = (unsigned char*)MAP_FAILED;
  mf->len = 0;
}


/* hex file format */

typedef struct hex_line
{
  struct hex_line* next;

  uint32_t addr;

  size_t size;
  uint8_t buf[1];

} hex_line_t;

static void hex_free_lines(hex_line_t* lines)
{
  hex_line_t* tmp;
  while (lines != NULL)
  {
    tmp = lines;
    lines = lines->next;
    free(tmp);
  }
}

static inline int is_mark(char c)
{
  switch (c)
  {
  case 0x00:
  case 0x01:
  case 0x04:
    return 1;
    break ;
  default:
    break ;
  }
  return 0;
}

static inline int is_hex(char c)
{
  if ((c >= '0') && (c <= '9')) return 1;
  if ((c >= 'a') && (c <= 'f')) return 1;
  if ((c >= 'A') && (c <= 'F')) return 1;
  return 0;
}

static int next_digit
(unsigned int* val, size_t len, const char** p, size_t* n)
{
  /* len the wanted digit length */

  char buf[16];
  unsigned int i;
  const char* s = *p;

  if (len >= sizeof(buf)) return -1;
  if (*n < len) return -1;

  /* copy check */
  for (i = 0; i < len; ++i, ++s)
  {
    if (is_hex(*s) == 0) return -1;
    buf[i] = *s;
  }

  /* convert */
  buf[i] = 0;
  *val = strtol(buf, NULL, 16);

  *n -= len;
  *p += len;

  return 0;
}

static int hex_read_lines(const char* filename, hex_line_t** first_line)
{
  int err = -1;
  mapped_file_t mf = MAPPED_FILE_INITIALIZER;
  const char* p;
  size_t n;
  unsigned int count;
  unsigned int off;
  unsigned int mark;
  unsigned int compl;
  unsigned int hiaddr = 0;
  unsigned int loaddr;
  hex_line_t* cur_line;
  hex_line_t* prev_line = NULL;

  *first_line = NULL;

  /* map the file */
  if (map_failed(&mf, filename)) goto on_error;
  p = (const char*)mf.base;
  n = mf.len;

  if (map_file(filename, &p, &n)) goto on_error;

  /* build line linked list */
  while (1)
  {
    /* skip col */
    if (n == 0) goto on_error;
    if (*p !=  ':') goto on_error;
    --n;
    ++p;

    /* data size */
    if (next_digit(&count, 2, &p, &n)) goto on_error;
    if (count == 0)
    {
      /* end of file */
      break ;
    }

    /* low addr */
    if (next_digit(&loaddr, 4, &p, &n)) goto on_error;

    /* block mark */
    if (next_digit(&mark, 2, &p, &n)) goto on_error;
    if (is_mark(mark) == 0) goto on_error;
    
    /* extended addressing */
    if (mark = 0x04)
    {
      if (next_digit(&hiaddr, 4, &p, &n)) goto on_error;
    }

    /* allocate and fill line */
    cur_line = malloc(offsetof(hex_line_t, buf) + count);
    cur_line->next = NULL;
    cur_line->addr = (hiaddr << 16) | loaddr;
    cur_line->size = count;

    /* link the new line */
    if (prev_line == NULL)
      *first_line = cur_line;
    else
      prev_line->next = cur_line;
    prev_line = cur_line;

    /* complement */
    if (next_digit(&compl, 2, &p, &n)) goto on_error;

    /* read data bytes */
    for (off = 0; off < count; ++off)
    {
      if (next_digit(&cur_line->buf[off], 2, &p, &n)) goto on_error;
    }

    /* todo: check complement */

    /* skip newline */
    if (n < 2) goto on_error;
    if ((p[0] != 0x0d) || (p[1] != 0x0a)) goto on_error;
    p += 2;
    n -= 2;

    /* end of file */
    if (mark == 0x00)
    {
      break ;
    }
  }

  /* success */
  *first_line = saved_line;
  *head = ;
  err = 0;

 on_error:
  if (mf.base != NULL) unmap_file(&mf);

  if ((err != 0) && *lines)
  {
    hex_free_lines(*lines);
    *lines = NULL;
  }

  return err;
}

/* main */

int main(int ac, char** av)
{
  /* command line: ./a.out <device> <file.hex> */

  const char* const devname = av[1];
  const char* const filename = av[2];

  hex_line_t* lines = NULL;

  if (hex_read_lines(filename, &lines) == -1)
    goto on_error;

  /* todo: initialize serial */

 on_error:
  if (lines != NULL) hex_free_lines(lines);

  return 0;
}
