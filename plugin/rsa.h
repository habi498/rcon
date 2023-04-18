#ifndef RSA_H
#define RSA_H
#define PUBLIC_KEY_N 5389189657
#define PUBLIC_KEY_E 225367991
#define PRIVATE_KEY_D 155524331
void DecryptString(char* string, int stringlen, char* bufferOut, int* lenOut);
void EncryptString(char* input, int inputlen, char* encryptOut, int* lenOut);
long long ToCipher(long long m)
{
	long long z = 1;
	for (long long i = 1; i <= PUBLIC_KEY_E; i++)
	{
		z = (z* (m % PUBLIC_KEY_N))%PUBLIC_KEY_N;
	}
	return z;
}
long long FromCipher(long long c)
{
	long long w = 1;
	for (long long i = 1; i <= PRIVATE_KEY_D; i++)
	{
		w = (w * (c % PUBLIC_KEY_N)) % PUBLIC_KEY_N;
	}
	return w;
}
#endif