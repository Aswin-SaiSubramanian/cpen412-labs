#include <stdio.h>
#include <string.h>
#include <ctype.h>

void main()
{
    unsigned long int start_addr = 0x08020000;

    // byte
    *((unsigned short int)start_addr) = 0xAA;
    *((unsigned short int)(start_addr + 1)) = 0xBB;

    // word
    *((unsigned short int)(start_addr + 2)) = 0xABCD;

    // long word
    *((unsigned short int)(start_addr + 4)) = 0x900FBA11;

    

    // unsigned short int byte_pattern[4] = {0xAA, 0xC4, 0x41, 0x15};
    // unsigned int word_pattern[4] = {0xAAAA, 0xC412, 0x4141, 0x1515};
    // unsigned long int lword_pattern[4] = {0xAAAAAAAA, 0xC412C412, 0x41414141, 0x15151515};
    // unsigned short int width_option = 0;
    // unsigned short int test_data_option = 0;
    // unsigned long int start_addr = 0;
    // unsigned long int end_addr = 0;

    // //scanflush();                       // flush any text that may have been typed ahead
    // printf("\r\nSelect value width for test [1 = bytes, 2 = words, 4 = long words]: ");
    // scanf("%u", &width_option);

    // switch(width_option) 
    // {
    //     case 1:
    //         printf("\nSelect test data [1 = AA, 2 = C4, 3 = 41, 4 = 15]: ");
    //         break;
    //     case 2:
    //         printf("\nSelect test data [1 = AAAA, 2 = C412, 3 = 4141, 4 = 1515]: ");
    //         break;
    //     case 3:
    //         printf("\nSelect test data [1 = AAAAAAAA, 2 = C412C412, 3 = 41414141, 4 = 15151515]: ");
    //         break;
    //     default:
    //         printf("\nInvalid value width\n");
    //         // early exit
    // }

    // scanf("%u", &test_data_option);

    // if (test_data_option > 4 || test_data_option < 1)
    // {
    //     printf("\nInvalid option selected");
    //     // deal with early exit
    // }

    // printf("\nEnter a start address for the test (if word or long word was selected, ensure this address is even): ");
    // scanf("%x", &start_addr);

    // printf("\nEnter an end address for the test (if word or long word was selected, ensure this address is even): ");
    // scanf("%x", &end_addr);

    // // word/longword
    // if (width_option != 'b' && (start_addr % 2 != 0 || end_addr % 2 != 0)) 
    // {
    //     printf("\nInvalid start or end address: must be even");
    //     // early exit
    // }

    // // check addr range 0802 0000 – 0803 0000 
    // if (start_addr < 0x08020000 || start_addr > 0x08030000 || end_addr < 0x08020000 || end_addr > 0x08030000)
    // {
    //     printf("\nInvalid start or end address: must be in 0x0802 0000 to 0x0803 0000 range");
    //     // early exit
    // }

    // // start addr <= end addr
    // if (start_addr > end_addr)
    // {
    //     printf("\nInvalid addresses: start address must be before or equal to end address");
    //     // early exit
    // }
    // printf("\nMade it!\n");
    // for (unsigned long int i = start_addr; i <= end_addr; i += width_option)
    // {
    //     // write
    //     (start_addr + width_option * i)

    //     // read


    //     // print progress
    //     if ((i - start_addr) % 2**10 == 0)
    //     {
    //         printf("Address: %d Data read/written: %d\n");
    //     }
    // }
}
