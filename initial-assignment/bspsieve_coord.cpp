#include <bsp.hpp>
#include <cstdio>
#include <cmath>
#include <unistd.h>

long P; // number of processors requested
long n;

void bspsieve_coord() {
  bsp_begin(P);
  double time0 = bsp_time();

  long p = bsp_nprocs(); // p = number of processors
  long s = bsp_pid();    // s = processor number

  long arrayLength = (long) ceil(((double) (n - 1)) / p);
  long blockStart = s*arrayLength + 2;

  bool *list = new bool[arrayLength]();
  long currPrimeAndCoord [2] = {2, 0};
  long lastCoordinator = 0;
  bsp_push_reg(currPrimeAndCoord, 2*sizeof(long));
  bsp_sync();

  double maxSievePrime = sqrt(n);

  while(currPrimeAndCoord[0] < maxSievePrime) {
    // Sieve, if the coordinator didn't change.
    if (lastCoordinator == currPrimeAndCoord[1]) {
      long p = currPrimeAndCoord[0];
      long j = p * p - blockStart;
      if (j < 0) {
        j = p - blockStart % p;
      }
      while (j < arrayLength) {
        list[j] = true;
        j += p;
      }
    }

    // Change current prime or coordinator
    if (s == currPrimeAndCoord[1]) {
      // Search for the next prime to sieve
      long j = currPrimeAndCoord[0] - blockStart + (lastCoordinator == currPrimeAndCoord[1]);
      while(j < arrayLength && list[j]) j++;
      currPrimeAndCoord[0] = blockStart + j;

      lastCoordinator = currPrimeAndCoord[1];
      currPrimeAndCoord[1] = j < arrayLength ? s : s + 1;

      long k = s+1;
      while (k < p) {
        bsp_put(k, currPrimeAndCoord, currPrimeAndCoord, 0, 2*sizeof(long));
        k++;
      }
      bsp_sync();
    } else {
      lastCoordinator = currPrimeAndCoord[1];
      bsp_sync();
    }
  }

  double time1 = bsp_time();
  if (s == 0)
    printf("coord needed %fs. \n", time1 - time0);

  /*for (long i = 0; i < n-1; i++) {
    if (i / arrayLength == s && !list[i % arrayLength]) {
      printf("%li is prime on proc %li.\n", i + 2, s);
    }
    bsp_sync();
  }*/

  bsp_pop_reg(currPrimeAndCoord);
  delete [] list;
  bsp_end();
}

int main(int argc, char **argv) {
  printf("How many processors do you want to use?\n");
  fflush(stdout);
  scanf("%ld", &P);
  if (P > bsp_nprocs()) {
    printf("Sorry, only %u processors available.\n",
           bsp_nprocs());
    fflush(stdout);
    exit(EXIT_FAILURE);
  }

  n = 10000000;

  bsp_init(bspsieve_coord, argc, argv);
  bspsieve_coord();
  exit(EXIT_SUCCESS);
}