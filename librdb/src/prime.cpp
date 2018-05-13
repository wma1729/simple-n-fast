/*
 * Finds the floor sqrt of a number. I think it is
 * faster than floor(sqrt(number)).
 */
static long
FloorSqrt(long number)
{
	long lo = 1;
	long hi = number;
	long mid;
	long sq;
	long res = 0;

	while (lo <= hi) {
		mid = (lo + hi) / 2;
		sq = mid * mid;
		if (sq < number) {
			lo = mid + 1;
			res = mid;
		} else if (sq == number) {
			res = mid;
			break;
		} else {
			hi = mid - 1;
		}
	}

	return res;
}

/**
 * Determines if a number is a prime.
 */
static int
IsPrime(long number)
{
	long i, ni;
	long sq;

	if ((number == 1) || (number == 2) || (number == 3))
	 	return 1;

	if ((number % 2) == 0)
		return 0;

	if ((number % 3) == 0)
		return 0;

	sq = FloorSqrt(number);

	i = 5;  /* start number */

	/*
 	 * ni is the next increment.
 	 * Following 6k - 1; 6k + 1 rule. All the prime numbers
 	 * follow the 6k - 1; 6k + 1 rule starting with 5.
 	 * Not all 6k - 1 and 6k + 1 numbers are prime but knowing
 	 * this fact, reduces the checks a lot.
	 * We increment the number by 2 then 4, 2 then 4 cycle.
 	 */

	ni = 2;

	while (i <= sq) {
		if ((number % i) == 0)
			return 0;

		i += ni;
		/*
		 * if (ni == 2)
 		 *	 ni = 4;
 		 * else if (ni == 4)
 		 *	 ni = 2;
		 *
		 * This result can be achieved more efficiently
 		 * using the following statement.
		 */
		ni = 6 - ni;
	}

	return 1;
}

/**
 * Given a number, finds the next prime number.
 * Input | Output
 * ------|-------
 *  12   | 13
 *  25   | 29
 *  512  | 521
 *  1024 | 1031
 */
int
NextPrime(int n)
{
	if (IsPrime(n)) {
		return n;
	}

	int ni = 2;
	int i = (n / 6) + 1;
	int next = (6 * i) - 1;

	while (next < n) {
		i++;
		next = (6 * i) - 1;
	}

	while (!IsPrime(next)) {
		next += ni;
		ni = 6 - ni;
	}

	return next;
}
