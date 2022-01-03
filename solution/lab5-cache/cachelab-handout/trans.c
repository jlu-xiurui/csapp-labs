/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
void trans2(int M, int N, int A[N][M], int B[M][N]);
void trans(int M, int N, int A[N][M], int B[M][N]);
void trans1(int M, int N, int A[N][M], int B[M][N]);
void trans3(int M, int N, int A[N][M], int B[M][N]);
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
 if(N == 64)
   trans3(M,N,A,B);
 else
   trans2(M,N,A,B);
}
/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}
char trans_desc1[] = "block_size = 8";
void trans1(int M, int N, int A[N][M], int B[M][N])
{
    int i,j,block_size = 8,ii;
    for(ii = 0;ii < N;ii += block_size){
      for(j = 0; j < M;j++){
        for(i = ii; i < N && i < ii+block_size;i++){
          B[j][i] = A[i][j];
        }
      }
  }
}
char transpose_desc2[] = "block size = 8 and register store";
void trans2(int M, int N, int A[N][M], int B[M][N])
{
  int i,j,t1,t2,t3,t4,t5,t6,t7,t8;
  for(i = 0;i < N-7;i+=8){
    for(j = 0; j < M;j++){
        t1 = A[i][j];
        t2 = A[i+1][j];
        t3 = A[i+2][j];
        t4 = A[i+3][j];
        t5 = A[i+4][j];
        t6 = A[i+5][j];
        t7 = A[i+6][j];
        t8 = A[i+7][j];
        B[j][i] = t1;
        B[j][i+1] = t2;
        B[j][i+2] = t3;
        B[j][i+3] = t4;
        B[j][i+4] = t5;
        B[j][i+5] = t6;
        B[j][i+6] = t7;
        B[j][i+7] = t8;
    }
  }
  for(;i<N;i++){
    for(j=0;j<M;j++){
      B[j][i] = A[i][j];
    }
  }
}
char transpose_desc3[] = "block size = 8*8 and register store";
void trans3(int M, int N, int A[N][M], int B[M][N])
{
  int i,j,ii,jj,t1,t2,t3,t4;
  for(i = 0;i < N;i+=8){
    for(j = 0;j < M;j+=8){
      for(jj = j;jj < j + 4;jj++){
        t1 = A[i][jj];
        t2 = A[i+1][jj];
        t3 = A[i+2][jj];
        t4 = A[i+3][jj];
        B[jj][i] = t1;
        B[jj][i+1] = t2;
        B[jj][i+2] = t3;
        B[jj][i+3] = t4;
      }
      for(ii = i;ii < i+8;ii++){
        t1 = A[ii][j+4]; t2 = A[ii][j+5];
        t3 = A[ii][j+6]; t4 = A[ii][j+7];
        B[j+4][ii] = t1; B[j+5][ii] = t2;
        B[j+6][ii] = t3; B[j+7][ii] = t4;
      }
      for(jj = j;jj < j + 4;jj++){
        t1 = A[i+4][jj];
        t2 = A[i+5][jj];
        t3 = A[i+6][jj];
        t4 = A[i+7][jj];
        B[jj][i+4] = t1;
        B[jj][i+5] = t2;
        B[jj][i+6] = t3;
        B[jj][i+7] = t4;
      }
    }
  }
}
/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 
    registerTransFunction(trans1, trans_desc1); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

