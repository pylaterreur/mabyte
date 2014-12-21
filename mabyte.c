#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  if (argc != 2)
    {
      fprintf(stderr, "Usage: %s object_file\n", argv[0]);
      return -1;
    }
  {
    int fd = open(argv[1], O_RDONLY);
    unsigned char e_ident[EI_NIDENT];
    if (sizeof(e_ident) != read(fd, e_ident, sizeof(e_ident)))
      {
	fprintf(stderr, "Read header error\n");
	return -1;
      }
    if (memcmp(e_ident, "\x7f""ELF", 4))
      {
	fprintf(stderr, "Not an ELF file\n");
	return -1;
      }
    if (e_ident[EI_CLASS] != ELFCLASS64)
      {
	fprintf(stderr, "Not an ELF64 file\n");
	return -1;	
      }
    if (-1 == lseek(fd, 0, SEEK_SET))
      {
	fprintf(stderr, "Failed to lseek at beginning of file");
	return -1;
      }
    {
      int i = 0;
      Elf64_Ehdr hdr64;
      if (sizeof(hdr64) != read(fd, &hdr64, sizeof(hdr64)))
	{
	  fprintf(stderr, "Read header 64 error\n");
	  return -1;
	}
      if (hdr64.e_shnum == 0)
	{
	  fprintf(stderr, "No section headers (or too many)\n");
	  return -1;
	}
      if (-1 == lseek(fd, hdr64.e_shoff, SEEK_SET))
	{
	  fprintf(stderr, "Failed to lseek at symbol headers");
	  return -1;
	}
      {
	int index = -1;
	char* names;
	Elf64_Shdr *shdr64;
	ssize_t size = hdr64.e_shnum * sizeof(*shdr64);
	shdr64 = malloc(size);
	if (size != read(fd, shdr64, size))
	  {
	    fprintf(stderr, "Error on section header read\n");
	  }
	names = malloc(shdr64[hdr64.e_shstrndx].sh_size + 1);
	names[shdr64[hdr64.e_shstrndx].sh_size] = '\0';
	if (-1 == lseek(fd, shdr64[hdr64.e_shstrndx].sh_offset, SEEK_SET))
	  {
	    fprintf(stderr, "Failed to lseek to string table");
	    return -1;
	  }
	if (shdr64[hdr64.e_shstrndx].sh_size != read(fd, names, shdr64[hdr64.e_shstrndx].sh_size))
	  {
	    fprintf(stderr, "Failed to read the string table");
	    return -1;
	  }
	for (i = 0; i < hdr64.e_shnum; ++i)
	  {
	    const char* const name = names + shdr64[i].sh_name;
	    if (!strcmp(".text", name))
	      {
		if (index != -1)
		  {
		    fprintf(stderr, "More than one .text section...");
		    return -1;
		  }
		index = i;
	      }
	  }
	if (index == -1)
	  {
	    fprintf(stderr, "Zero .text section...");
	    return -1;
	  }
	free(names); /* cause we don't care about freeing if we got a return -1 before... */
	{
	  /* no more error management for a POC... */
	  size_t size = shdr64[index].sh_size;
	  char *text = malloc(size);
	  lseek(fd, shdr64[index].sh_offset, SEEK_SET);
	  read(fd, text, size);
	  write(1, text, size);
	}
      }
    }
  }
  return 0;
}
