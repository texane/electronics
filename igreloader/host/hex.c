#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


/* dspic33f related */

#define FLASH_PAGE_SIZE (512 * 3)

static inline uint32_t get_mem_flags(uint32_t addr, uint16_t size)
{
#define MEM_FLAG_USER (1 << 0) /* config otherwise */
#define MEM_FLAG_VECTOR (1 << 1)
#define MEM_FLAG_FLASH (1 << 2)
#define MEM_FLAG_DCR (1 << 3) /* device config register */
#define MEM_FLAG_DEVID (1 << 4)
#define MEM_FLAG_RESERVED (1 << 5)

  const uint32_t hi = addr + size;

  uint32_t flags = 0;

  if (hi <= 0x800000)
  {
    /* user memory space */
    flags |= MEM_FLAG_USER;
    if (hi <= 0x200) flags |= MEM_FLAG_VECTOR;
    else if (hi <= 0x15800) flags |= MEM_FLAG_FLASH;
    else flags |= MEM_FLAG_RESERVED;
  }
  else
  {
    /* config memory space */
    if ((addr >= 0xf80000) && (hi <= 0xf80018)) flags |= MEM_FLAG_DCR;
    else if ((addr >= 0xff0000) && (hi <= 0xff0002)) flags |= MEM_FLAG_DEVID;
    else flags |= MEM_FLAG_RESERVED;
  }

  return flags;
}


/* endianness */

#define CONFIG_USE_LENDIAN 1

#if CONFIG_USE_LENDIAN

static inline uint16_t read_uint16(uint8_t* s)
{
  return *(uint16_t*)s;
}

static inline uint32_t read_uint32(uint8_t* s)
{
  return *(uint32_t*)s;
}

static inline void write_uint16(uint8_t* s, uint16_t x)
{
  *(uint16_t*)s = x;
}

static inline void write_uint32(uint8_t* s, uint32_t x)
{
  *(uint32_t*)s = x;
}

#else /* local is big endian */

static inline uint16_t read_uint16(uint8_t* s)
{
  /* todo */
}

static inline uint32_t read_uint32(uint8_t* s)
{
  /* todo */
}

static inline void write_uint16(uint8_t* s, uint16_t x)
{
  /* todo */
}

static inline void write_uint32(uint8_t* s, uint32_t x)
{
  /* todo */
}

#endif /* CONFIG_USE_LENDIAN */


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
  if (mf->base == MAP_FAILED) goto on_error;

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

typedef struct hex_range
{
  struct hex_range* next;
  struct hex_range* prev;

  uint32_t addr;

  size_t size;
  uint8_t buf[1];

} hex_range_t;

static void hex_free_ranges(hex_range_t* ranges)
{
  hex_range_t* tmp;
  while (ranges != NULL)
  {
    tmp = ranges;
    ranges = ranges->next;
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

static inline int skip_newline(const char** p, size_t* n)
{
#if 0 /* 0x0d 0x0a sequence */
  if (*n < 2) return -1;
  if (((*p)[0] != 0x0d) || ((*p)[1] != 0x0a)) goto on_error;
  *p += 2;
  *n -= 2;
#else
  if ((*n) == 0) return -1;
  if ((*p)[0] != 0x0a) return -1;
  *p += 1;
  *n -= 1;
#endif

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

static int hex_read_ranges(const char* filename, hex_range_t** first_range)
{
  int err = -1;
  mapped_file_t mf = MAPPED_FILE_INITIALIZER;
  const char* p;
  size_t n;
  unsigned int data;
  unsigned int count;
  unsigned int off;
  unsigned int mark;
  unsigned int compl;
  unsigned int hiaddr = 0;
  unsigned int loaddr;
  hex_range_t* cur_range;
  hex_range_t* prev_range = NULL;

  *first_range = NULL;

  /* map the file */
  if (map_file(&mf, filename)) goto on_error;
  p = (const char*)mf.base;
  n = mf.len;

  /* build range linked list */
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

    if (mark == 0x00)
    {
      /* data range */

      /* allocate and fill range */
      cur_range = malloc(offsetof(hex_range_t, buf) + count);
      cur_range->next = NULL;
      cur_range->prev = prev_range;
      cur_range->addr = (hiaddr << 16) | loaddr;
      cur_range->size = count;

      /* link the new range */
      if (prev_range == NULL)
	*first_range = cur_range;
      else
	prev_range->next = cur_range;
      prev_range = cur_range;

      /* read data bytes */
      for (off = 0; off < count; ++off)
      {
	if (next_digit(&data, 2, &p, &n)) goto on_error;
	cur_range->buf[off] = data;
      }
    }
    else if (mark == 0x04)
    {
      /* extended addressing */
      if (next_digit(&hiaddr, 4, &p, &n)) goto on_error;
    }
    else if (mark == 0x01)
    {
      /* end of file */
      break ;
    }

    /* complement */
    if (next_digit(&compl, 2, &p, &n)) goto on_error;
    /* todo: check complement */

    /* skip newrange */
    skip_newline(&p, &n);
  }

  /* success */
  err = 0;

 on_error:
  if (mf.base != NULL) unmap_file(&mf);

  if ((err != 0) && *first_range)
  {
    hex_free_ranges(*first_range);
    *first_range = NULL;
  }

  return err;
}

static inline void swap_ptrs(void** a, void** b)
{
  void* const tmp = *a;
  *a = *b;
  *b = tmp;
}

static void hex_sort_ranges(hex_range_t** ranges)
{
  /* sort by addr, assume non overlapping ranges */

  hex_range_t* i;
  hex_range_t* next_i;
  hex_range_t* j;
  hex_range_t* k;

  for (i = *ranges; i != NULL; i = next_i)
  {
    k = i;
    next_i = i->next;

    for (j = next_i; j != NULL; j = j->next)
      if (j->addr < k->addr) k = j;

    /* swap if smaller found */
    if (i != k)
    {
      /* head special case */
      if (i->prev == NULL) *ranges = k;
      else i->prev->next = k;

      /* tail special case */
      if (k->next != NULL) k->next->prev = i;

      /* neighbor nodes special case */
      if (next_i != k)
      {
	i->next->prev = k;
	k->prev->next = i;
	swap_ptrs((void**)&i->prev, (void**)&k->prev);
	swap_ptrs((void**)&i->next, (void**)&k->next);
      }
      else
      {
	i->next = k->next;
	k->next = i;
	k->prev = i->prev;
	i->prev = k;
      }
    }
  }
}

#if 0 /* unused, broken */

static void hex_paginate_ranges(hex_range_t** ranges)
{
  /* convert from basic hex format to pages */

  /* current non paged range */
  hex_range_t* pos = NULL;

  /* first, last non paged ranges in the paged range */
  hex_range_t* first;
  hex_range_t* pos;

  /* offset from first->addr */
  size_t first_off;

  /* offset from new_range->buf */
  size_t new_off;

  /* offset from pos->addr */
  size_t pos_off;

  /* newly paged range */
  hex_range_t* new_range;

  /* previous paged range */
  hex_range_t* prev_range = NULL;

  /* new paged list head */
  hex_range_t* new_head = NULL;

  /* current paged range size */
  size_t sumed_size;

  /* working variables */
  size_t tmp;
  size_t i;

  /* algorithm assumes ranges sorted */
  hex_sort_ranges(ranges);

  sumed_size = 0;
  first_off = 0;

 split_range:

  /* add to sumed_size and adjust */
  sumed_size += pos->size;
  if (pos == first) sumed_size -= first_off;
  if (sumed_size > FLASH_PAGE_SIZE) sumed_size = FLASH_PAGE_SIZE;

  if ((pos->next == NULL) || (sumed_size == FLASH_PAGE_SIZE))
  {
    /* create a new range */
    new_range = malloc(offsetof(hex_range_t, buf), sumed_size);
    new_range->next = NULL;
    new_range->prev = prev_range;
    new_range->addr = first_range->addr + first_off;
    new_range->size = sumed_size;
    if (new_head == NULL) new_head = new_range;
    else prev_range->next = new_range;
    prev_range = new_range;

    /* copy [first->addr + off, first->addr + sumed_size] */
    pos = first;
    pos_off = 0; /* avoid warning */
    new_off = 0;
    while (sumed_size)
    {
      tmp = pos->size;

      pos_off = 0;
      if (pos == first)
      {
	pos_off = first_off;
	tmp -= first_off;
      }

      if (tmp > sumed_size) tmp = sumed_size;

      memcpy(new_range->buf + new_off, pos->buf + pos_off, tmp);

      sumed_size -= tmp;
      new_off += tmp;

      if (tmp == (pos->size - pos_off)) pos = pos->next;
    }

    /* update and continue in the current non paged range */
    first = pos;
    first_off = pos_off;
    sumed_size = 0;
    goto split_range;
  }
  else if (pos_size)
  {
    /* add to sumed_size and consume next range */
    sumed_size += pos_size;
    pos = pos->next;
    goto split_range;
  }
  /* else, done */

  /* release no longer used ranges */
  hex_free_ranges(*ranges);
  *ranges = new_head;
}

#endif /* unused, broken */

static void hex_merge_ranges(hex_range_t** ranges)
{
  /* merge the contiguous ranges */

  hex_range_t* saved_ranges;

  hex_range_t* first;
  hex_range_t* last;
  hex_range_t* pos;

  hex_range_t* cur_range;
  hex_range_t* prev_range = NULL;

  size_t off;
  size_t size;

  hex_sort_ranges(ranges);

  first = *ranges;
  saved_ranges = *ranges;
  *ranges = NULL;

  while (first != NULL)
  {
    /* find [first, last[ */
    size = first->size;
    for (last = first->next; last != NULL; last = last->next)
    {
      if (last->addr != (last->prev->addr + last->prev->size)) break ;
      size += last->size;
    }

    /* create a merged range */
    cur_range = malloc(offsetof(hex_range_t, buf) + size);
    cur_range->next = NULL;
    cur_range->prev = prev_range;
    cur_range->addr = first->addr;
    cur_range->size = size;
    if (prev_range == NULL) *ranges = cur_range;
    else prev_range->next = cur_range;
    prev_range = cur_range;

    off = 0;
    for (pos = first; pos != last; pos = pos->next)
    {
      memcpy(cur_range->buf + off, pos->buf, pos->size);
      off += pos->size;
    }

    first = last;
  }

  hex_free_ranges(saved_ranges);
}

__attribute__((unused))
static int hex_check_ranges(const hex_range_t* ranges)
{
  return 0;
}

#if 1 /* unused */

#include <stdio.h>

__attribute__((unused))
static void hex_print_ranges(const hex_range_t* ranges)
{
  for (; ranges != NULL; ranges = ranges->next)
  {
    printf("[ %x - %x [ (%x)\n",
	   ranges->addr, ranges->addr + ranges->size, ranges->size);
  }
}

#endif /* unused */


/* write hex file to flash */

static int do_program(void* dev, const char* filename)
{
  /* filename a hex file */

  hex_range_t* ranges;
  hex_range_t* pos;
  size_t off;
  size_t i;
  size_t page_size;
  uint8_t buf[8];

  if (hex_read_ranges(filename, &ranges) == -1)
  {
    printf("invalid hex file\n");
    goto on_error;
  }

  hex_print_ranges(ranges);
  printf("--\n");

  hex_merge_ranges(&ranges);

  /* for each range, write pages */
  for (pos = ranges; pos != NULL; pos = pos->next)
  {
    /* handle unaligned first page */
    off = 0;
    page_size = FLASH_PAGE_SIZE - (pos->addr % FLASH_PAGE_SIZE);

    printf("range [%x - %x[: 0x%x\n",
	   pos->addr, pos->addr + pos->size,
	   get_mem_flags(pos->addr, pos->size));

    while (1)
    {
      /* adjust page size */
      page_size = FLASH_PAGE_SIZE;
      if ((off + page_size) > pos->size) page_size = pos->size - off;

      /* todo: check device addr range */

      /* initiate write sequence */
#define CMD_ID_WRITE_PROGRAM 0
      buf[0] = CMD_ID_WRITE_PROGRAM;
      write_uint32(buf + 1, pos->addr + off);
      write_uint16(buf + 5, page_size);

      /* todo: com_send(dev, buf); */

      /* todo: wait for command ack */

      /* send the page 8 bytes at a time */
      for (i = 0; i < pos->size; i += 8)
      {
	/* todo: com_send(dev, pos->buf + off + i); */

	/* todo: wait for data ack */
      }

      /* todo: wait for page programming ack */

      /* next page or done */
      off += page_size;
      if (off == pos->size) break ;
    }
  }

 on_error:
  if (ranges != NULL) hex_free_ranges(ranges);

  return 0;
}


/* main */

#include <stdio.h>

int main(int ac, char** av)
{
  /* command line: ./a.out <device> <file.hex> */

  const char* const devname = av[1];
  const char* const filename = av[2];

  /* todo: initialize serial */

  /* program device flash */
  if (do_program(NULL, filename) == -1)
  {
    printf("programming failed\n");
    return -1;
  }

  return 0;
}
