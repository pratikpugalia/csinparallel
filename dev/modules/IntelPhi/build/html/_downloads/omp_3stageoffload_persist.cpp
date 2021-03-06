// Copyright 2003-2012 Intel Corporation. All Rights Reserved.
// 
// The source code contained or described herein and all documents related 
// to the source code ("Material") are owned by Intel Corporation or its
// suppliers or licensors.  Title to the Material remains with Intel Corporation
// or its suppliers and licensors.  The Material is protected by worldwide
// copyright and trade secret laws and treaty provisions.  No part of the
// Material may be used, copied, reproduced, modified, published, uploaded,
// posted, transmitted, distributed, or disclosed in any way without Intel's
// prior express written permission.
// 
// No license under any patent, copyright, trade secret or other intellectual
// property right is granted to or conferred upon you by disclosure or delivery
// of the Materials, either expressly, by implication, inducement, estoppel
// or otherwise.  Any license under such intellectual property rights must
// be express and approved by Intel in writing.



/* System headers */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#include <vector>
#include "malloc.h"
#include "offload.h"

#include "timer.h"

#define iteration 1
#define RETAIN free_if(0)
#define REUSE alloc_if(0)
#define ALLOC alloc_if(1)
#define FREE free_if(1)

#define PHIDEV 0
using namespace std;


/*declare the matrices both on teh host and the target*/
__attribute__((target(mic))) float *A = NULL;
__attribute__((target(mic))) float *B = NULL;
__attribute__((target(mic))) float *C = NULL;

/*---------------------------------------------------------*/

__attribute__((target(mic))) void offload_check(void)
{
#ifdef __MIC__
  printf("Code running on MIC\n");
#else
  printf("Code running on host\n");
#endif
}

/*----------------------------------------------------------*/

int main(int argc, char **argv)
{

	int N; /* Matrix dimensions */
	int matrix_bytes; /* Matrix size in bytes */
	int matrix_elements; /* Matrix size in elements */
	int Nthreads;

	/* Check command line arguments */
	if (argc != 2) {
		printf("\nUsage: %s <N> \n\n", argv[0]);
		return -1;
	}

	/* Parse command line arguments */
	N = atoi(argv[1]);
	if (N <= 0) {
		printf("Invalid matrix size\n");
		return -1;
	}

	
	matrix_elements = N * N;
	matrix_bytes = sizeof(float) * matrix_elements;

	/* Allocate the matrices */
	A = (float *)malloc(matrix_bytes);
	if (A == NULL) {
		printf("Could not allocate matrix A\n");
		return -1;
	}

	B = (float *)malloc(matrix_bytes);
	if (B == NULL) {
		printf("Could not allocate matrix B\n");
		return -1;
	}

	C = (float *)malloc(matrix_bytes);
	if (C == NULL) {
		printf("Could not allocate matrix C\n");
		return -1;
	}

	/* Initialize the matrices */
	#pragma novector
	for (int i = 0; i < matrix_elements; i++) {
		A[i] = 1.0; B[i] = 2.0; C[i] = 0.0;
	}

	float secs = 0.0f;
	float transfer_time = 0.0f;


	for(int i = 0; i<iteration; i++){
		/*re-initialize the output marrix	*/
		#pragma novector
		for (int i = 0; i < matrix_elements; i++) {
                	C[i] = 0.0;
        	}
		
		/*call the compute matrix mul function*/
		int iMaxThreads;

        	/*transfer data to the coprocessor and let it persist using free_if(0) specifier*/
        	#pragma offload target(mic:PHIDEV) in(A:length(N*N) RETAIN) in(B:length(N*N) RETAIN) in(C:length(N*N) RETAIN)
        	{
        	}

         	/*resuse the data on the card using alloc_if(0) free_if(0) specifier */
    		#pragma offload target(mic:PHIDEV) nocopy(A:length(N*N) REUSE RETAIN) nocopy(B:length(N*N) REUSE RETAIN) nocopy(C:length(N*N) REUSE RETAIN)
    		{
                	offload_check();

        		iMaxThreads = omp_get_max_threads();

        		printf("matrix multiplication running with %d threads\n", iMaxThreads);

        		#pragma omp parallel for
            		for (int j = 0; j < N; ++j)
                		for (int k = 0; k < N; ++k)
                    			for (int i = 0; i < N; ++i)
                        			C[j * N + i] += A[k * N + i] * B[j * N + k];
    		}


         	/*transfer the output matrix to host and free all the data on the card using free_if(1)*/
        	#pragma offload target(mic:PHIDEV) nocopy(A:length(N*N) FREE) nocopy(B:length(N*N) FREE) out(C:length(N*N) REUSE FREE)
    		{
    		}

		
	}
	
	//fprintf(stderr, "\tavg time : %f s \n", secs/iteration);
	/* Display the result */
	/*for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++)
			printf("%7.3f ", C[j + i * N]);
		printf("\n");
	}*/
	/* Free the matrices */
	free(A); free(B); free(C);

    return 0;
}

