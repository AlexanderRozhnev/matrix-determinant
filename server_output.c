#include "server_output.h"

#include <stdio.h>

void PrintOutputs(const struct ServerOutput* output_data) {
    printf("Det. = %.8e\n", output_data->determinant);
    if (output_data->full_data) {
        printf("Avg. det. = %.8e\n", output_data->avg_determinant);
	    printf("Del. det = %.8e\n", output_data->prev_Nth_determinant);
    } else {
        printf("Avg. det. = N/A\n");
	    printf("Del. det = N/A\n");
    }
    printf("\n");
}