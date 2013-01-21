// CAST-128.cpp : Defines the entry point for the console application.
#include "stdafx.h"
#include "cast_s.h"

UINT32 S[9][256];

// ============================================================================
// ==============================================================================

void InitSubstitutionBoxes(void)
{
	memcpy(S[1], CAST_S_table0, sizeof(CAST_S_table0));
	memcpy(S[2], CAST_S_table1, sizeof(CAST_S_table1));
	memcpy(S[3], CAST_S_table2, sizeof(CAST_S_table2));
	memcpy(S[4], CAST_S_table3, sizeof(CAST_S_table3));
	memcpy(S[5], CAST_S_table4, sizeof(CAST_S_table4));
	memcpy(S[6], CAST_S_table5, sizeof(CAST_S_table5));
	memcpy(S[7], CAST_S_table6, sizeof(CAST_S_table6));
	memcpy(S[8], CAST_S_table7, sizeof(CAST_S_table7));
}

// ============================================================================
// ==============================================================================
void plaintext2L0_R0(BYTE plaintext[8], UINT32 &L0, UINT32 &R0)
{
	L0 = *(UINT32 *) (plaintext + 4);
	R0 = *(UINT32 *) (plaintext);
}

// ============================================================================
// ==============================================================================
UINT32 fourByte2uint32(BYTE byte0, BYTE byte1, BYTE byte2, BYTE byte3)
{
	//~~~~~~~~~~~~~~~~~~
	UINT32 u32ret = byte0;
	//~~~~~~~~~~~~~~~~~~

	u32ret <<= 8;
	u32ret |= byte1;
	u32ret <<= 8;
	u32ret |= byte2;
	u32ret <<= 8;
	u32ret |= byte3;
	return u32ret;
};

// ============================================================================
// ==============================================================================

UINT32 byteArr2uint(const BYTE byte[16], int iByte)
{
	assert(iByte <= 12);
	return fourByte2uint32(byte[iByte], byte[iByte + 1], byte[iByte + 2], byte[iByte + 3]);
}

// ============================================================================
// ==============================================================================
void uint2fourByte(UINT32 uint, OUT BYTE byte[16], int iByte)
{
	assert(iByte <= 12);
	byte[iByte] = (uint & 0xff000000) >> 24;
	byte[iByte + 1] = (uint & 0x00ff0000) >> 16;
	byte[iByte + 2] = (uint & 0x0000ff00) >> 8;
	byte[iByte + 3] = uint & 0x000000ff;
}

// ============================================================================
//    zx[izx]zx[izx+1]zx[izx+2]zx[izx+3] = xz[ixz]xz[ixz+1]xz[ixz+2]xz[ixz+3] ^
//    S5[iS5] ^ S6[iS6] ^S7 [iS7] ^ S8[iS8] ^ SiS[iSi];
// ============================================================================
void CaluZX(OUT BYTE zx[16],
			int izx,
			IN const BYTE xz[16],
			int ixz,
			int iS5,
			int iS6,
			int iS7,
			int iS8,
			int iS,
			int iSi)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	UINT32 xz4byte = byteArr2uint(xz, ixz);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	printf("index %d-%d xz4byte = %x\n", ixz, ixz + 4, xz4byte);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	UINT32 zx4byte = xz4byte ^ S[5][iS5] ^ S[6][iS6] ^ S[7][iS7] ^ S[8][iS8] ^ S[iS][iSi];
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	printf("zx4byte(openssl-l) %x =  %x^%x^%x^%x^%x^%x\n", zx4byte, xz4byte, S[5][iS5], S[6][iS6], S[7][iS7], S[8][iS8],
		   S[iS][iSi]);

	uint2fourByte(zx4byte, zx, izx);
}

// ============================================================================
//    K = S5[izx5] ^ S6[izx6] ^ S7[izx7] ^ S8[izx8] ^ Si[izxi];
// ============================================================================
UINT32 CaluK(IN const BYTE zx[16], int izxS5, int izxS6, int izxS7, int izxS8, int iS, int izxsSi)
{
	return S[5][zx[izxS5]] ^ S[6][zx[izxS6]] ^ S[7][zx[izxS7]] ^ S[8][zx[izxS8]] ^ S[iS][zx[izxsSi]];
}

// ============================================================================
// ==============================================================================
void CaluK(IN const BYTE key[16], OUT UINT32 Km[16], OUT UINT32 Kr[16])
{
	//~~~~~~~~~
	BYTE z[16];
	BYTE x[16];
	UINT32 K[17];
	//~~~~~~~~~

	// The subkeys are formed from the key x0x1x2x3x4x5x6x7x8x9xAxBxCxDxExF
	// as follows.
	memcpy(x, key, 16);

	// z0z1z2z3 = x0x1x2x3 ^ S5[xD] ^ S6[xF] ^ S7[xC] ^ S8[xE] ^ S7[x8];
	// z4z5z6z7 = x8x9xAxB ^ S5[z0] ^ S6[z2] ^ S7[z1] ^ S8[z3] ^ S8[xA];
	// z8z9zAzB = xCxDxExF ^ S5[z7] ^ S6[z6] ^ S7[z5] ^ S8[z4] ^ S5[x9];
	// zCzDzEzF = x4x5x6x7 ^ S5[zA] ^ S6[z9] ^ S7[zB] ^ S8[z8] ^ S6[xB];
	CaluZX(z, 0, x, 0, x[0xD], x[0xF], x[0xC], x[0xE], 7, x[0x8]);
	CaluZX(z, 4, x, 8, z[0], z[2], z[1], z[3], 8, x[0xA]);
	CaluZX(z, 8, x, 0xC, z[7], z[6], z[5], z[4], 5, x[9]);
	CaluZX(z, 0xC, x, 4, z[0xA], z[9], z[0xB], z[8], 6, x[0xB]);

	for (int i = 0; i < 16; ++i) {
		printf("z %d %x\n", i, z[i]);
	}

	// K1 = S5[z8] ^ S6[z9] ^ S7[z7] ^ S8[z6] ^ S5[z2];
	// K2 = S5[zA] ^ S6[zB] ^ S7[z5] ^ S8[z4] ^ S6[z6];
	// K3 = S5[zC] ^ S6[zD] ^ S7[z3] ^ S8[z2] ^ S7[z9];
	// K4 = S5[zE] ^ S6[zF] ^ S7[z1] ^ S8[z0] ^ S8[zC];
	K[1] = CaluK(z, 8, 9, 7, 6, 5, 2);
	K[2] = CaluK(z, 0xA, 0xB, 5, 4, 6, 6);
	K[3] = CaluK(z, 0xC, 0xD, 3, 2, 7, 9);
	K[4] = CaluK(z, 0xE, 0xF, 1, 0, 8, 0xC);

	// x0x1x2x3 = z8z9zAzB ^ S5[z5] ^ S6[z7] ^ S7[z4] ^ S8[z6] ^ S7[z0];
	// x4x5x6x7 = z0z1z2z3 ^ S5[x0] ^ S6[x2] ^ S7[x1] ^ S8[x3] ^ S8[z2];
	// x8x9xAxB = z4z5z6z7 ^ S5[x7] ^ S6[x6] ^ S7[x5] ^ S8[x4] ^ S5[z1];
	// xCxDxExF = zCzDzEzF ^ S5[xA] ^ S6[x9] ^ S7[xB] ^ S8[x8] ^ S6[z3];
	CaluZX(x, 0, z, 8, z[5], z[7], z[4], z[6], 7, z[0]);
	CaluZX(x, 4, z, 0, x[0], x[2], x[1], x[3], 8, z[2]);
	CaluZX(x, 8, z, 4, x[7], x[6], x[5], x[4], 5, z[1]);
	CaluZX(x, 0xC, z, 0xC, x[0xA], x[9], x[0xB], x[8], 6, z[3]);

	// K5 = S5[x3] ^ S6[x2] ^ S7[xC] ^ S8[xD] ^ S5[x8];
	// K6 = S5[x1] ^ S6[x0] ^ S7[xE] ^ S8[xF] ^ S6[xD];
	// K7 = S5[x7] ^ S6[x6] ^ S7[x8] ^ S8[x9] ^ S7[x3];
	// K8 = S5[x5] ^ S6[x4] ^ S7[xA] ^ S8[xB] ^ S8[x7];
	K[5] = CaluK(x, 3, 2, 0xC, 0xD, 5, 8);
	K[6] = CaluK(x, 1, 0, 0xE, 0xF, 6, 0xD);
	K[7] = CaluK(x, 7, 6, 8, 9, 7, 3);
	K[8] = CaluK(x, 5, 4, 0xA, 0xB, 8, 7);

	// z0z1z2z3 = x0x1x2x3 ^ S5[xD] ^ S6[xF] ^ S7[xC] ^ S8[xE] ^ S7[x8];
	// z4z5z6z7 = x8x9xAxB ^ S5[z0] ^ S6[z2] ^ S7[z1] ^ S8[z3] ^ S8[xA];
	// z8z9zAzB = xCxDxExF ^ S5[z7] ^ S6[z6] ^ S7[z5] ^ S8[z4] ^ S5[x9];
	// zCzDzEzF = x4x5x6x7 ^ S5[zA] ^ S6[z9] ^ S7[zB] ^ S8[z8] ^ S6[xB];
	CaluZX(z, 0, x, 0, x[0xD], x[0xF], x[0xC], x[0xE], 7, x[8]);
	CaluZX(z, 4, x, 8, z[0], z[2], z[1], z[3], 8, x[0xA]);
	CaluZX(z, 8, x, 0xC, z[7], z[6], z[5], z[4], 5, x[9]);
	CaluZX(z, 0xC, x, 4, z[0xA], z[9], z[0xB], z[8], 6, x[0xB]);

	// K9 = S5[z3] ^ S6[z2] ^ S7[zC] ^ S8[zD] ^ S5[z9];
	// K10 = S5[z1] ^ S6[z0] ^ S7[zE] ^ S8[zF] ^ S6[zC];
	// K11 = S5[z7] ^ S6[z6] ^ S7[z8] ^ S8[z9] ^ S7[z2];
	// K12 = S5[z5] ^ S6[z4] ^ S7[zA] ^ S8[zB] ^ S8[z6];
	K[9] = CaluK(z, 3, 2, 0xC, 0xD, 5, 9);
	K[10] = CaluK(z, 1, 0, 0xE, 0xF, 6, 0xC);
	K[11] = CaluK(z, 7, 6, 8, 9, 7, 2);
	K[12] = CaluK(z, 5, 4, 0xA, 0xB, 8, 6);

	// x0x1x2x3 = z8z9zAzB ^ S5[z5] ^ S6[z7] ^ S7[z4] ^ S8[z6] ^ S7[z0];
	// x4x5x6x7 = z0z1z2z3 ^ S5[x0] ^ S6[x2] ^ S7[x1] ^ S8[x3] ^ S8[z2];
	// x8x9xAxB = z4z5z6z7 ^ S5[x7] ^ S6[x6] ^ S7[x5] ^ S8[x4] ^ S5[z1];
	// xCxDxExF = zCzDzEzF ^ S5[xA] ^ S6[x9] ^ S7[xB] ^ S8[x8] ^ S6[z3];
	CaluZX(x, 0, z, 8, z[5], z[7], z[4], z[6], 7, z[0]);
	CaluZX(x, 4, z, 0, x[0], x[2], x[1], x[3], 8, z[2]);
	CaluZX(x, 8, z, 4, x[7], x[6], x[5], x[4], 5, z[1]);
	CaluZX(x, 0xC, z, 0xC, x[0xA], x[9], x[0xB], x[8], 6, z[3]);

	// K13 = S5[x8] ^ S6[x9] ^ S7[x7] ^ S8[x6] ^ S5[x3];
	// K14 = S5[xA] ^ S6[xB] ^ S7[x5] ^ S8[x4] ^ S6[x7];
	// K15 = S5[xC] ^ S6[xD] ^ S7[x3] ^ S8[x2] ^ S7[x8];
	// K16 = S5[xE] ^ S6[xF] ^ S7[x1] ^ S8[x0] ^ S8[xD];
	K[13] = CaluK(x, 8, 9, 7, 6, 5, 3);
	K[14] = CaluK(x, 0xA, 0xB, 5, 4, 6, 7);
	K[15] = CaluK(x, 0xC, 0xD, 3, 2, 7, 8);
	K[16] = CaluK(x, 0xE, 0xF, 1, 0, 8, 0xD);

	for (int i = 1; i <= 16; ++i) {
		printf("K %d : %x\n", i, K[i]);
	}
}

// ============================================================================
// ==============================================================================
int _tmain(int argc, _TCHAR *argv[])
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	const int ROUND = 16;
	UINT32 L[ROUND + 1];
	UINT32 R[ROUND + 1];
	// INPUT: plaintext m1...m64;
	// key K = k1...k128.;
	BYTE plaintext[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
	BYTE key[16] = { 0x01, 0x23, 0x45, 0x67, 0x12, 0x34, 0x56, 0x78, 0x23, 0x45, 0x67, 0x89, 0x34, 0x56, 0x78, 0x9A };
	// 1. (key schedule) Compute 16 pairs of subkeys {Kmi, Kri} from K;
	UINT32 Km[16];
	UINT32 Kr[16];
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	plaintext2L0_R0(plaintext, L[0], R[0]);

	InitSubstitutionBoxes();

	for (int i = 1; i <= ROUND; ++i) {
		L[i] = R[i - 1];
	}

	CaluK(key, Km, Kr);

	//~~~~~~~~~~~~~
	char szLine[256];
	//~~~~~~~~~~~~~

	while (gets(szLine));

	return 0;
}
