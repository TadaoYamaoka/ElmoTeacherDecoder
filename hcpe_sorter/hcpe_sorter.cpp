#include "position.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>
#include <random>
#include <string>

int main(int argc, char** argv)
{
	if (argc < 3) {
		return 1;
	}

	int *num = new int[argc - 2];
	int sum_num = 0;

	for (int i = 1; i < argc - 1; i++) {
		int fd = open(argv[i], O_RDONLY);
		if (fd == -1) {
			fprintf(stderr, "open error\n");
			exit(1);
		}
		struct stat stbuf;
		fstat(fd, &stbuf);
		int num_tmp = stbuf.st_size / sizeof(HuffmanCodedPosAndEval);

		num[i - 1] = num_tmp;
		sum_num += num_tmp;

		close(fd);
	}

	HuffmanCodedPosAndEval *hcpe_vec_all = new HuffmanCodedPosAndEval[sum_num + 1];
	HuffmanCodedPosAndEval *hcpe_vec = hcpe_vec_all;

	for (int i = 0; i < argc - 2; i++) {

		FILE *fp = fopen(argv[i + 1], "rb");
		if (fp == NULL) {
			fprintf(stderr, "fopen error\n");
			exit(1);
		}

		if (fread(hcpe_vec, sizeof(HuffmanCodedPosAndEval), num[i], fp) != num[i]) {
			fprintf(stderr, "fread error\n");
			exit(1);
		}

		hcpe_vec += num[i];

		fclose(fp);
	}

	// shuffle
	std::shuffle(hcpe_vec_all, hcpe_vec_all + sum_num, std::mt19937());

	hcpe_vec = hcpe_vec_all;
	std::string outdir = argv[argc - 1];
	for (int i = 0; i < argc - 2; i++) {
		char fname[256];
		_splitpath(argv[i + 1], NULL, NULL, fname, NULL);
		FILE *fp = fopen((outdir + "/" + fname).c_str(), "wb");
		if (fp == NULL) {
			fprintf(stderr, "fopen error\n");
			exit(1);
		}

		if (fwrite(hcpe_vec, sizeof(HuffmanCodedPosAndEval), num[i], fp) == 0) {
			fprintf(stderr, "fread error\n");
			exit(1);
		}

		hcpe_vec += num[i];

		fclose(fp);
	}

	return 0;
}