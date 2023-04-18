#include "rsa.h"
#include <stdio.h>
int main()
{
	long long x;
	printf("Enter m:");
	scanf("%llu", &x);
	printf("\nTo Cipher: %llu",ToCipher(x));
	printf("\nEnter c:");
	scanf("%llu", &x);
	printf("\nFrom Cipher: %llu", FromCipher(x));
}
