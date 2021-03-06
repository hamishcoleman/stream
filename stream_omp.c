/*-----------------------------------------------------------------------*/
/* Program: Stream                                                       */
/* Revision: $Id: stream_omp.c,v 5.4 2009/02/19 13:57:12 mccalpin Exp mccalpin $ */
/* Original code developed by John D. McCalpin                           */
/* Programmers: John D. McCalpin                                         */
/*              Joe R. Zagar                                             */
/*                                                                       */
/* This program measures memory transfer rates in MB/s for simple        */
/* computational kernels coded in C.                                     */
/*-----------------------------------------------------------------------*/
/* Copyright 1991-2003: John D. McCalpin                                 */
/*-----------------------------------------------------------------------*/
/* License:                                                              */
/*  1. You are free to use this program and/or to redistribute           */
/*     this program.                                                     */
/*  2. You are free to modify this program for your own use,             */
/*     including commercial use, subject to the publication              */
/*     restrictions in item 3.                                           */
/*  3. You are free to publish results obtained from running this        */
/*     program, or from works that you derive from this program,         */
/*     with the following limitations:                                   */
/*     3a. In order to be referred to as "STREAM benchmark results",     */
/*         published results must be in conformance to the STREAM        */
/*         Run Rules, (briefly reviewed below) published at              */
/*         http://www.cs.virginia.edu/stream/ref.html                    */
/*         and incorporated herein by reference.                         */
/*         As the copyright holder, John McCalpin retains the            */
/*         right to determine conformity with the Run Rules.             */
/*     3b. Results based on modified source code or on runs not in       */
/*         accordance with the STREAM Run Rules must be clearly          */
/*         labelled whenever they are published.  Examples of            */
/*         proper labelling include:                                     */
/*         "tuned STREAM benchmark results"                              */
/*         "based on a variant of the STREAM benchmark code"             */
/*         Other comparable, clear and reasonable labelling is           */
/*         acceptable.                                                   */
/*     3c. Submission of results to the STREAM benchmark web site        */
/*         is encouraged, but not required.                              */
/*  4. Use of this program or creation of derived works based on this    */
/*     program constitutes acceptance of these licensing restrictions.   */
/*  5. Absolutely no warranty is expressed or implied.                   */
/*-----------------------------------------------------------------------*/
# include <stdio.h>
# include <math.h>
# include <float.h>
# include <limits.h>
# include <sys/time.h>
#include <omp.h>
#include <string.h>
#include <stdlib.h>

/* INSTRUCTIONS:
 *
 *	1) Stream requires a good bit of memory to run.  Adjust the
 *          value of 'ELEMENTS' (below) to give a 'timing calibration' of
 *          at least 20 clock-ticks.  This will provide rate estimates
 *          that should be good to about 5% precision.
 */

long ELEMENTS = 2000000;
# define NTIMES	10
# define OFFSET	0

/*
 *	3) Compile the code with full optimization.  Many compilers
 *	   generate unreasonably bad code before the optimizer tightens
 *	   things up.  If the results are unreasonably good, on the
 *	   other hand, the optimizer might be too smart for me!
 *
 *         Try compiling with:
 *               cc -O stream_omp.c -o stream_omp
 *
 *         This is known to work on Cray, SGI, IBM, and Sun machines.
 *
 *
 *	4) Mail the results to mccalpin@cs.virginia.edu
 *	   Be sure to include:
 *		a) computer hardware model number and software revision
 *		b) the compiler flags
 *		c) all of the output from the test case.
 * Thanks!
 *
 */

# define HLINE "-------------------------------------------------------------\n"

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

static double	*a, *b, *c;

static double	avgtime[4] = {0}, maxtime[4] = {0},
		mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};

static char	*label[4] = {"Copy:      ", "Scale:     ",
    "Add:       ", "Triad:     "};

double	bytes[4];

extern double mysecond();
int checkSTREAMresults();
#ifdef TUNED
extern void tuned_STREAM_Copy();
extern void tuned_STREAM_Scale(double scalar);
extern void tuned_STREAM_Add();
extern void tuned_STREAM_Triad(double scalar);
#endif

#define qprintf(level, fmt, args...) \
    if (verbose>=level) { \
        printf(fmt, ##args); \
    }

int verbose = 1;

int
main(int argc, char **argv)
    {
    int			quantum, checktick();
    int			BytesPerWord;
    register int	j, k;
    double		scalar, t, times[4][NTIMES];

    if (argc>=2 && strcmp(argv[1], "quiet")==0) {
        verbose = 0;
    }
    if (argc>=3) {
        ELEMENTS = atol(argv[2]);
    }

    a = malloc(sizeof(double)*(ELEMENTS+OFFSET));
    b = malloc(sizeof(double)*(ELEMENTS+OFFSET));
    c = malloc(sizeof(double)*(ELEMENTS+OFFSET));

    if (a == NULL) { perror("malloc"); }
    if (b == NULL) { perror("malloc"); }
    if (c == NULL) { perror("malloc"); }

    bytes[0] = 2 * sizeof(double) * ELEMENTS;
    bytes[1] = 2 * sizeof(double) * ELEMENTS;
    bytes[2] = 3 * sizeof(double) * ELEMENTS;
    bytes[3] = 3 * sizeof(double) * ELEMENTS;

    /* --- SETUP --- determine precision and check timing --- */

    qprintf(1,HLINE);
    BytesPerWord = sizeof(double);
    qprintf(1,"This system uses %d bytes per DOUBLE PRECISION word.\n",
	BytesPerWord);

    qprintf(1,HLINE);
    printf("Array size = %d, Offset = %d\n" , ELEMENTS, OFFSET);
    printf("Total memory required = %.1f MB.\n",
	(3.0 * BytesPerWord) * ( (double) ELEMENTS / 1048576.0));
    qprintf(1,"Each test is run %d times, but only\n", NTIMES);
    qprintf(1,"the *best* time for each is used.\n");

#ifdef _OPENMP
    qprintf(1,HLINE);
#pragma omp parallel private(k)
    {
        k = omp_get_num_threads();
        if (omp_get_thread_num() == 0) {
            printf ("Number of Threads requested = %i\n",k);
        }
    }
#endif

    /* Get initial value for system clock. */
#pragma omp parallel for
    for (j=0; j<ELEMENTS; j++) {
	a[j] = 1.0;
	b[j] = 2.0;
	c[j] = 0.0;
	}

    qprintf(1,HLINE);

    if  ( (quantum = checktick()) >= 1) {
	qprintf(1,"Your clock granularity/precision appears to be "
	    "%d microseconds.\n", quantum);
    } else {
	printf("Your clock granularity appears to be "
	    "less than one microsecond.\n");
    }

    t = mysecond();
#pragma omp parallel for
    for (j = 0; j < ELEMENTS; j++)
	a[j] = 2.0E0 * a[j];
    t = 1.0E6 * (mysecond() - t);

    qprintf(1,"Each test below will take on the order"
	" of %d microseconds.\n", (int) t  );
    qprintf(1,"   (= %d clock ticks)\n", (int) (t/quantum) );
    qprintf(1,"Increase the size of the arrays if this shows that\n");
    qprintf(1,"you are not getting at least 20 clock ticks per test.\n");

    qprintf(1,HLINE);

    qprintf(1,"WARNING -- The above is only a rough guideline.\n");
    qprintf(1,"For best results, please be sure you know the\n");
    qprintf(1,"precision of your system timer.\n");
    qprintf(1,HLINE);
    
    /*	--- MAIN LOOP --- repeat test cases NTIMES times --- */

    scalar = 3.0;
    for (k=0; k<NTIMES; k++)
	{
	times[0][k] = mysecond();
#ifdef TUNED
        tuned_STREAM_Copy();
#else
#pragma omp parallel for
	for (j=0; j<ELEMENTS; j++)
	    c[j] = a[j];
#endif
	times[0][k] = mysecond() - times[0][k];
	
	times[1][k] = mysecond();
#ifdef TUNED
        tuned_STREAM_Scale(scalar);
#else
#pragma omp parallel for
	for (j=0; j<ELEMENTS; j++)
	    b[j] = scalar*c[j];
#endif
	times[1][k] = mysecond() - times[1][k];
	
	times[2][k] = mysecond();
#ifdef TUNED
        tuned_STREAM_Add();
#else
#pragma omp parallel for
	for (j=0; j<ELEMENTS; j++)
	    c[j] = a[j]+b[j];
#endif
	times[2][k] = mysecond() - times[2][k];
	
	times[3][k] = mysecond();
#ifdef TUNED
        tuned_STREAM_Triad(scalar);
#else
#pragma omp parallel for
	for (j=0; j<ELEMENTS; j++)
	    a[j] = b[j]+scalar*c[j];
#endif
	times[3][k] = mysecond() - times[3][k];
	}

    /*	--- SUMMARY --- */

    for (k=1; k<NTIMES; k++) /* note -- skip first iteration */
	{
	for (j=0; j<4; j++)
	    {
	    avgtime[j] = avgtime[j] + times[j][k];
	    mintime[j] = MIN(mintime[j], times[j][k]);
	    maxtime[j] = MAX(maxtime[j], times[j][k]);
	    }
	}
    
    printf("Function      Rate (MB/s)   Avg time     Min time     Max time\n");
    for (j=0; j<4; j++) {
	avgtime[j] = avgtime[j]/(double)(NTIMES-1);

	printf("%s%11.4f  %11.4f  %11.4f  %11.4f\n", label[j],
	       1.0E-06 * bytes[j]/mintime[j],
	       avgtime[j],
	       mintime[j],
	       maxtime[j]);
    }
    qprintf(1,HLINE);

    /* --- Check Results --- */
    int r = checkSTREAMresults();
    qprintf(1,HLINE);

    return r;
}

# define	M	20

int
checktick()
    {
    int		i, minDelta, Delta;
    double	t1, t2, timesfound[M];

/*  Collect a sequence of M unique time values from the system. */

    for (i = 0; i < M; i++) {
	t1 = mysecond();
	while( ((t2=mysecond()) - t1) < 1.0E-6 )
	    ;
	timesfound[i] = t1 = t2;
	}

/*
 * Determine the minimum difference between these M values.
 * This result will be our estimate (in microseconds) for the
 * clock granularity.
 */

    minDelta = 1000000;
    for (i = 1; i < M; i++) {
	Delta = (int)( 1.0E6 * (timesfound[i]-timesfound[i-1]));
	minDelta = MIN(minDelta, MAX(Delta,0));
	}

   return(minDelta);
    }



/* A gettimeofday routine to give access to the wall
   clock timer on most UNIX-like systems.  */

#include <sys/time.h>

double mysecond()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

int checkSTREAMresults ()
{
	double aj,bj,cj,scalar;
	double asum,bsum,csum;
	double epsilon;
	int	j,k;

    /* reproduce initialization */
	aj = 1.0;
	bj = 2.0;
	cj = 0.0;
    /* a[] is modified during timing check */
	aj = 2.0E0 * aj;
    /* now execute timing loop */
	scalar = 3.0;
	for (k=0; k<NTIMES; k++)
        {
            cj = aj;
            bj = scalar*cj;
            cj = aj+bj;
            aj = bj+scalar*cj;
        }
	aj = aj * (double) (ELEMENTS);
	bj = bj * (double) (ELEMENTS);
	cj = cj * (double) (ELEMENTS);

	asum = 0.0;
	bsum = 0.0;
	csum = 0.0;
	for (j=0; j<ELEMENTS; j++) {
		asum += a[j];
		bsum += b[j];
		csum += c[j];
	}
#ifdef VERBOSE
	printf ("Results Comparison: \n");
	printf ("        Expected  : %f %f %f \n",aj,bj,cj);
	printf ("        Observed  : %f %f %f \n",asum,bsum,csum);
#endif

#define abs(a) ((a) >= 0 ? (a) : -(a))
	epsilon = 1.e-8;

	if (abs(aj-asum)/asum > epsilon) {
		printf ("Failed Validation on array a[]\n");
		printf ("        Expected  : %f \n",aj);
		printf ("        Observed  : %f \n",asum);
                return 1;
	}
	else if (abs(bj-bsum)/bsum > epsilon) {
		printf ("Failed Validation on array b[]\n");
		printf ("        Expected  : %f \n",bj);
		printf ("        Observed  : %f \n",bsum);
                return 1;
	}
	else if (abs(cj-csum)/csum > epsilon) {
		printf ("Failed Validation on array c[]\n");
		printf ("        Expected  : %f \n",cj);
		printf ("        Observed  : %f \n",csum);
                return 1;
	}
	else {
		printf ("Solution Validates\n");
                return 0;
	}
}

void tuned_STREAM_Copy()
{
	int j;
#pragma omp parallel for
        for (j=0; j<ELEMENTS; j++)
            c[j] = a[j];
}

void tuned_STREAM_Scale(double scalar)
{
	int j;
#pragma omp parallel for
	for (j=0; j<ELEMENTS; j++)
	    b[j] = scalar*c[j];
}

void tuned_STREAM_Add()
{
	int j;
#pragma omp parallel for
	for (j=0; j<ELEMENTS; j++)
	    c[j] = a[j]+b[j];
}

void tuned_STREAM_Triad(double scalar)
{
	int j;
#pragma omp parallel for
	for (j=0; j<ELEMENTS; j++)
	    a[j] = b[j]+scalar*c[j];
}
