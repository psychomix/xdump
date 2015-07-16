/* 
 * xdump.c
 * Copyright (c) 2015 by Gergely Tamas <dice@psychomix.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>

#include <openssl/md5.h>

int usage (char *prg) {
  fprintf (stderr, "usage: %s [-v, -s bytes, -w bytes, -t (hex, dec), -l bytes, -m 0xAABBCCDD] file\n", prg);
  exit (-1);
}

int print_pos (unsigned char dl, unsigned char d_type, unsigned long POS) {
  switch (d_type) {
    case 1:
      printf ("%08lx | ", POS);
      break;
    case 2:
      printf ("%*ld | ", dl, POS);
      break;
  }

  return 0;
}

int print_data (unsigned char d_type, int data) {
  switch (d_type) {
    case 1:
      if (data < 0) printf ("   "); else printf ("%02x", data);
      break;
    case 2:
      if (data < 0) printf ("    "); else printf ("%3d", data);
      break;
  }

  return 0;
}

int print_str (unsigned int *j, unsigned long *POS, unsigned char STR[], unsigned int STR_len, unsigned char d_type) {
  if ((*j)>0) {
    (*POS) += (*j);
    STR[(*j)] = 0;
    for (;(*j)<STR_len;(*j)++) print_data (d_type, -1);
    printf (" | %s\n", STR);
  }

  return 0;
}

int main (int argc, char **argv) {
  int fd, opt;
  struct stat sb;
  unsigned char *addr, md5[16];

  char data[2];
  unsigned char *STR, verbose = 0, *cmp_buffer = NULL, m, d_type = 2, dl = 1; // default: dec, 20 chars
  unsigned int j, STR_len = 20, cmp_bpos = 0, k;
  unsigned long i, POS = 0, ds, max = 0, skip = 0;

  while ((opt = getopt(argc, argv, "vs:w:l:m:t:")) != -1) {
    switch (opt) {
      case 'v':
        verbose = 1;
        break;
      case 's':
        if (!strncasecmp (optarg, "0x", 2)) {
          skip = strtoul(optarg, NULL, 16);
        } else {
          skip = atol(optarg);
        }
        break;
      case 'w':
        STR_len = atoi(optarg);
        break;
      case 'l':
        if (!strncasecmp (optarg, "0x", 2)) {
          max = strtoul(optarg, NULL, 16);
        } else {
          max = atol(optarg);
        }
        break;
      case 'm':
        cmp_buffer = (unsigned char *)malloc (strlen(optarg)/2);
        for (i=0; i<strlen(optarg)/2; i++) {
          memcpy (data, &optarg[i*2], 2);
          if (!strncasecmp(data, "0x", 2)) continue;
          cmp_buffer[cmp_bpos ++] = (unsigned char)(strtoull(data, NULL, 16));
        }
        break;
      case 't':
        if (!strncasecmp (optarg, "hex", 3)) d_type = 1;
        if (!strncasecmp (optarg, "dec", 3)) d_type = 2;
        break;
      default:
        usage (*argv);
    }
  }

  if (optind >= argc) usage (*argv);

  STR = (unsigned char *)malloc (STR_len);

  fd = open (argv[optind], O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "error: unable to open %s\n", argv[optind]);
    exit (-1);
  }

  if (verbose) fprintf (stderr, "%s\n", argv[optind]);

  printf ("\n# name : %s\n", argv[optind]);

  if (fstat (fd, &sb) == -1) {
    fprintf (stderr, "error: unable to stat fd\n");
    exit (-1);
  }

  printf ("# size : %ld bytes\n", sb.st_size);

  if (!sb.st_size) {
    printf ("# skipped ...\n");
    exit (0);
  }

  addr = mmap (NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (addr == MAP_FAILED) {
    fprintf (stderr, "error: mmap failed\n");
    exit (-1);
  }

  printf ("# md5  : ");
  MD5(addr, sb.st_size, md5);
  for (i=0; i<16; i++) printf ("%02x", md5[i]);
  printf ("\n\n");

  ds = sb.st_size;
  while (ds > 10) { ds /= 10; dl ++; }

  if (skip > 0) {
    POS += skip;
  }

  print_pos (dl, d_type, POS);

  for (i=skip, j=0; i<sb.st_size; i++) { // default: skip=0
    if (max > 0 && i > skip+max) break;

    if (j > 0 && cmp_buffer) {
      m = 0;
      for (k=0; k<cmp_bpos; k++) {
        if (addr[i+k] == cmp_buffer[k]) m++;
      }
      if (m == 4) {
        print_str (&j, &POS, STR, STR_len, d_type);
        j = 0;
        printf ("\n");
        print_pos (dl, d_type, POS);
      }
    }

    if (j>0) printf (" ");

    print_data (d_type, addr[i]);
    
    if (isprint (addr[i])) STR[j] = addr[i]; else STR[j] = ' ';
    j ++;
    if (j == STR_len) {
      print_str (&j, &POS, STR, STR_len, d_type);
      j = 0;
      print_pos (dl, d_type, POS);
    }
  }

  print_str (&j, &POS, STR, STR_len, d_type);

  return 0;
}
