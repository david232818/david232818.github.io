//
// TDES: A "tiny" version of DES
// for illustrating various attacks
//
// Block size: 16 bits
// Key size: 16 bits
// Number of S-Boxes: 2
// Each S-Box maps 6 bits to 4 (as in DES)
// Number of rounds: 4
//
//   One round of TDES
//
//    L                  R
//  L0L1L2L3L4L5L6L7    R0R1R2R3R4R5R6R7
//    |                / |                         key = (k0,k1,...,k15)
//    |              /   |                 k0k1k2k3k4k5k6k7   k8k9k10k11k12k13k14k15
//    |            /     |                  rotate left 2         rotate left 1
//    | 8        /       | 8             select bits 023457  select bits 9,10,11,13,14,15
//    |        /         |                           \              /
//    |      /           |                            \  6         / 6
//    |    /             |                             \         /
//    |    |             |                              \      /
//    |    |          expand                              \  /
//    |    |  R4R7R2R1R5R7R0R2R6R5R0R3                     |
//    |    |             |                                 |
//    |    |             |                                 |
//    |    |             | 12                              | 12
//    |    |             |                                 |
//    |    |             |                                 |
//    |    |            XOR -------------------------------
//    |    |             |
//    |    |             |
//    |    | 8           | 12
//    |    |             |
//    |    |             |
//    |    |          S-boxes
//    |    |             |
//    |    |             |
//    |    |             | 8
//    |    |             |
//    |    |             |
//     ---------------- XOR
//         |             |
//        /  8           | 8
//      /                |
//     L                 R
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// defines
#define NUMBER_OF_ROUNDS 4

// function prototypes
void printBits(int plaintext, int len);
int sboxLeft(int x);
int sboxRight(int x);
int atoh(char *x, int len);
int genSubKey(int key, int encryptSubKey[], int decryptSubKey[], int numberOfRounds);
int TDES(int data, int subKey[], int encrypt, int numberOfRounds);
int expand(int right);


int main(int argc, const char *argv[])
{
    int key,
        numberOfRounds,
        plaintext,
        ciphertext,
        decryptedtext;

    int encryptSubKey[NUMBER_OF_ROUNDS],
        decryptSubKey[NUMBER_OF_ROUNDS];

    int encrypt = 1,
        decrypt = 0;

    if(argc != 3)
    {
oops:   fprintf(stderr, "\nUsage: %s key input\n\n", argv[0]);
        fprintf(stderr, "where key == key in hex (16 bits)\n");
        fprintf(stderr, "     input == one block of input in hex (16 bits)\n\n");
        exit(0);
    }

    if((strlen(argv[1]) != 4) || (strlen(argv[2]) != 4))
    {
        fprintf(stderr, "\nError --- key and plaintext are 16 bits (4 hex characters)\n\n");
		goto oops;
    }

    key = atoh((char *)argv[1], 4);
    
    plaintext = atoh((char *)argv[2], 4);
   
    numberOfRounds = NUMBER_OF_ROUNDS;
        
    printf("\nkey        = %04x = ", key);
    printBits(key, 16);

    printf("\nplaintext  = %04x = ", plaintext);
    printBits(plaintext, 16);
    printf("\n");

    //
    // generate the subkey (both encryption and decryption)
    //
    genSubKey(key, encryptSubKey, decryptSubKey, numberOfRounds);

    //
    // encrypt the plaintext
    //
    ciphertext = TDES(plaintext, encryptSubKey, encrypt, numberOfRounds);

    printf("ciphertext = %04x = ", ciphertext);
    printBits(ciphertext, 16);
    printf("\n");

    //
    // decrypt the ciphertext
    //    
    decryptedtext = TDES(ciphertext, decryptSubKey, decrypt, numberOfRounds);

    printf("decrypt    = %04x = ", decryptedtext);
    printBits(decryptedtext, 16);
    printf("\n\n");
        
} // end main


//
// TDES --- tiny DES
//
int TDES(int data, int subKey[], int flag, int numberOfRounds)
{
    int left,
        right,
        newLeft,
        newRight,
        round,
        expandRight,
        expandPlusSubKey,
        sboxOutput,
        output;

    if(flag) // encryption
    {
        // bits 0 thru 7
        left = (data & 0xff00) >> 8;
        // bits 8 thru 15
        right = data & 0xff;
    }
    else // decryption
    {
        // we swap left and right halves before
        // beginning decryption, so that same
        // code can be used for encryption
        // and decryption (only change is that
        // subkey must be used in reverse order)

        // bits 0 thru 7
        right = (data & 0xff00) >> 8;
        // bits 8 thru 15
        left = data & 0xff;
    }

    for(round = 0; round < numberOfRounds; ++round)
    {
        // new left half = current right half
        newLeft = right;
        
        // expansion permutation of right half
        // x0x1x2...x11
        expandRight = expand(right);

        // XOR subkey
        expandPlusSubKey = expandRight ^ subKey[round];
        
        // output of S-Boxes
        sboxOutput = (sboxLeft((expandPlusSubKey >> 6) & 0x3f) << 4)
                   | sboxRight(expandPlusSubKey & 0x3f);
        
        // XOR with left
        newRight = left ^ sboxOutput;

        // left and right for next round
        left = newLeft;
        right = newRight;

    }// next round

    if(flag) // encryption
    {
        output = ((left << 8) | (right & 0xff)) & 0xffff;
    }
    else // decryption
    {
        // swap the halves to get the plaintext
        output = ((right << 8) | (left & 0xff)) & 0xffff;
    }
    
    return(output);

}// end TDES


//
// expansion permutation
//
int expand(int right)
{
    int expanded;

    expanded = ((right & 0x08) << 8)  //  x0 = r4
             | ((right & 0x01) << 10) //  x1 = r7
             | ((right & 0x20) << 4)  //  x2 = r2
             | ((right & 0x40) << 2)  //  x3 = r1
             | ((right & 0x04) << 5)  //  x4 = r5
             | ((right & 0x01) << 6)  //  x5 = r7
             | ((right & 0x80) >> 2)  //  x6 = r0
             | ((right & 0x20) >> 1)  //  x7 = r2
             | ((right & 0x02) << 2)  //  x8 = r6
             | ((right & 0x04) << 0)  //  x9 = r5
             | ((right & 0x80) >> 6)  // x10 = r0
             | ((right & 0x10) >> 4); // x11 = r3

    return(expanded);
}


//
// generate subKey
//
int genSubKey(int key, int encryptSubKey[], int decryptSubKey[], int numberOfRounds)
{
    int keyLeft,
        keyRight,
        i;

    //
    // generate encryptSubKey
    //
    keyLeft = ((key & 0xff00) >> 8) & 0xff;
    keyRight = key & 0xff;
    for(i = 0; i < numberOfRounds; ++i)
    {
        // rotate left by 2
        keyLeft = ((keyLeft << 2) | ((keyLeft & 0xc0) >> 6)) & 0xff;
        
        // rotate left by 1
        keyRight = ((keyRight << 1) | ((keyRight & 0x80) >> 7)) & 0xff;
        
        encryptSubKey[i] 
            = ((keyLeft  & 0x80) << 4) // bit 0
            | ((keyLeft  & 0x20) << 5) // bit 2
            | ((keyLeft  & 0x10) << 5) // bit 3
            | ((keyLeft  & 0x08) << 5) // bit 4
            | ((keyLeft  & 0x04) << 5) // bit 5
            | ((keyLeft  & 0x01) << 6) // bit 7
            | ((keyRight & 0x70) >> 1) // bits 9 10 and 11
            |  (keyRight & 0x07);      // bits 13, 14 and 15

    }// next i

    //
    // generate decryptSubKey
    //
    for(i = 0; i < numberOfRounds; ++i)
    {
        decryptSubKey[i] = encryptSubKey[numberOfRounds - i - 1];
    }

}// end genSubKey


//
// left S-Box
//
int sboxLeft(int x)
{
    static int sbox[4][16] = {
        {0x6,0x9,0xa,0x3,0x4,0xd,0x7,0x8,0xe,0x1,0x2,0xb,0x5,0xc,0xf,0x0},
        {0x9,0xe,0xb,0xa,0x4,0x5,0x0,0x7,0x8,0x6,0x3,0x2,0xc,0xd,0x1,0xf},
        {0x8,0x1,0xc,0x2,0xd,0x3,0xe,0xf,0x0,0x9,0x5,0xa,0x4,0xb,0x6,0x7},
        {0x9,0x0,0x2,0x5,0xa,0xd,0x6,0xe,0x1,0x8,0xb,0xc,0x3,0x4,0x7,0xf}};

    int row,
        column,
        y;
    
    row = ((x & 0x20) >> 4) | (x & 0x1);
    column = (x >> 1) & 0xf;
    y = sbox[row][column];
    
    return y;
    
}// end sboxLeft


//
// right S-Box
//
int sboxRight(int x)
{
    static int sbox[4][16] = {
        {0xc,0x5,0x0,0xa,0xe,0x7,0x2,0x8,0xd,0x4,0x3,0x9,0x6,0xf,0x1,0xb},
        {0x1,0xc,0x9,0x6,0x3,0xe,0xb,0x2,0xf,0x8,0x4,0x5,0xd,0xa,0x0,0x7},
        {0xf,0xa,0xe,0x6,0xd,0x8,0x2,0x4,0x1,0x7,0x9,0x0,0x3,0x5,0xb,0xc},
        {0x0,0xa,0x3,0xc,0x8,0x2,0x1,0xe,0x9,0x7,0xf,0x6,0xb,0x5,0xd,0x4}};

    int row,
        column,
        y;
    
    row = ((x & 0x20) >> 4) | (x & 0x1);
    column = (x >> 1) & 0xf;
    y = sbox[row][column];
    
    return y;
    
}// end sboxRight


//
// utility to print the bits
//
void printBits(int x, int len)
{
    int i;
    for(i = 0; i < len; ++i)
    {
        printf("%1d", (x >> (len - i - 1)) & 0x1);
    }
    
}// end printBits


//
// convert characters 0 thru 9, A thru F and a thru f to hex
//
int atoh(char *x, int len)
{
    int i,
        ans;
    
    ans = 0;
    for(i = 0; i < len; ++i)
    {
        if((x[i] >= 48) && (x[i] <= 57))
        {
            ans |= ((int)x[i] - 48) << 4*(len - i - 1);
            continue;
        }
        if((x[i] >= 65) && (x[i] <= 70))
        {
            ans |= ((int)x[i] - 55) << 4*(len - i - 1);
            continue;
        }
        if((x[i] >= 97) && (x[i] <= 102))
        {
            ans |= ((int)x[i] - 87) << 4*(len - i - 1);
            continue;
        }
        fprintf(stderr, "\nError in atoh\n\n");
        
    }// next i
    
    return(ans);

}// end atoh
