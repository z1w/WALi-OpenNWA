// C4B output: |[x,n]|+|[y,m]|

#include "tick.h"

void start(int x, int n, int y, int m)
{
	while (x < n) {
	        tick(1);
		while (y < m && __VERIFIER_nondet_int()) {
			tick(1);
			y = y + 1;
		}
		x = x + 1;
	}
}

int main() 
{
	init_tick(0);

	int x = __VERIFIER_nondet_int();
	int y = __VERIFIER_nondet_int();
	int n = __VERIFIER_nondet_int();
	int m = __VERIFIER_nondet_int();

	start(x, n, y, m);
	
	int bnd = ((n > x) ? (n - x) : 0) + ((m > y) ? (m - y) : 0);
	assert(__cost <= bnd);
	
	return 0;
}
