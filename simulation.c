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

	FILE *input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        fprintf(stderr, "Wrong File format.\n", input_filename);
        return 1;
}

    FILE *output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        fprintf(stderr, "Wrong File format.\n", output_filename);
        fclose(input_file); 
        return 1;
}
}
