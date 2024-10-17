#include <stdio.h>
#include <stdlib.h>

// Initializing structure

typedef struct {
    int process_id;
    int page_num;
    int last_accessed;
} memory;


// Main function
int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "usage: %s in.txt out.txt\n", argv[0]);
	}
	return 0;

	char *in_file_name = argv[1];
    char *out_file_name = argv[2];

	FILE *in_file = fopen(in_file_name, "r");
    if (in_file == NULL) {
        fprintf(stderr, "Wrong File format.\n", in_file_name);
        return 1;
}

    FILE *out_file = fopen(out_file_name, "w");
    if (out_file == NULL) {
        fprintf(stderr, "Wrong File format.\n", out_file_name);
        fclose(in_file); 
        return 1;
}

}
