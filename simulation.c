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
}
