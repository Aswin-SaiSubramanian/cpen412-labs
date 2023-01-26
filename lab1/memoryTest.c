#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*********************************************************************************************
**	RS232 port addresses
*********************************************************************************************/

#define RS232_Control     *(volatile unsigned char *)(0x00400040)
#define RS232_Status      *(volatile unsigned char *)(0x00400040)
#define RS232_TxData      *(volatile unsigned char *)(0x00400042)
#define RS232_RxData      *(volatile unsigned char *)(0x00400042)
#define RS232_Baud        *(volatile unsigned char *)(0x00400044)


/*********************************************************************************************************
**  Subroutine to provide a low level output function to 6850 ACIA
**  This routine provides the basic functionality to output a single character to the serial Port
**  to allow the board to communicate with HyperTerminal Program
**
**  NOTE you do not call this function directly, instead you call the normal putchar() function
**  which in turn calls _putch() below). Other functions like puts(), printf() call putchar() so will
**  call _putch() also
*********************************************************************************************************/

int _putch( int c)
{
    while((RS232_Status & (char)(0x02)) != (char)(0x02))    // wait for Tx bit in status register or 6850 serial comms chip to be '1'
        ;

    RS232_TxData = (c & (char)(0x7f));                      // write to the data register to output the character (mask off bit 8 to keep it 7 bit ASCII)
    return c ;                                              // putchar() expects the character to be returned
}

/*********************************************************************************************************
**  Subroutine to provide a low level input function to 6850 ACIA
**  This routine provides the basic functionality to input a single character from the serial Port
**  to allow the board to communicate with HyperTerminal Program Keyboard (your PC)
**
**  NOTE you do not call this function directly, instead you call the normal getchar() function
**  which in turn calls _getch() below). Other functions like gets(), scanf() call getchar() so will
**  call _getch() also
*********************************************************************************************************/
int _getch( void )
{
    char c ;
    while((RS232_Status & (char)(0x01)) != (char)(0x01))    // wait for Rx bit in 6850 serial comms chip status register to be '1'
        ;

    return (RS232_RxData & (char)(0x7f));                   // read received character, mask off top bit and return as 7 bit ASCII character
}

// converts hex char to 4 bit binary equiv in range 0000-1111 (0-F)
// char assumed to be a valid hex char 0-9, a-f, A-F

char xtod(int c)
{
    if ((char)(c) <= (char)('9'))
        return c - (char)(0x30);    // 0 - 9 = 0x30 - 0x39 so convert to number by sutracting 0x30
    else if((char)(c) > (char)('F'))    // assume lower case
        return c - (char)(0x57);    // a-f = 0x61-66 so needs to be converted to 0x0A - 0x0F so subtract 0x57
    else
        return c - (char)(0x37);    // A-F = 0x41-46 so needs to be converted to 0x0A - 0x0F so subtract 0x37
}


void main()
{  
    unsigned int pattern[4] = {0,0,0,0};
    unsigned char * pattern_input = "";
    unsigned int width_option = 0;
    unsigned int start_addr = 0;
    unsigned int end_addr = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int value_from_mem = 0;



    // GET USER INPUTS

    scanflush();                       // flush any text that may have been typed ahead
    printf("\r\nSelect value width for test [1 = bytes, 2 = words, 4 = long words]: ");
    scanf("%u", &width_option);

    for (i = 0; i < 4; i++) 
    {
        printf("\nPlease enter hexadecimal string 1 of 4, of length %u characters: ", width_option*2);
        scanf("%s", pattern_input);
        
        j = 0;
        if (pattern_input[j] == "\0") 
        {
            printf("\nHexadecimal string is not long enough");
            goto early_exit;
        }
        pattern[i] = xtod(pattern_input[j]);
        for (j = 1; j < 2*width_option; j++) 
        {
            if (pattern_input[j] == "\0") 
            {
                printf("\nHexadecimal string is not long enough");
                goto early_exit;
            }
            pattern[i] = (pattern[i] << 4) | xtod(pattern_input[j]); 
        }
        
        printf("\npattern %u is %x", i, pattern[i]);
    }

    early_exit: printf("\nFAIL");
    

    // switch(width_option) 
    // {
    //     case 1:
    //         printf("\r\nSelect test data [1 = AA, 2 = C4, 3 = 41, 4 = 15]: ");
    //         break;
    //     case 2:
    //         printf("\r\nSelect test data [1 = AAAA, 2 = C412, 3 = 4141, 4 = 1515]: ");
    //         break;
    //     case 4:
    //         printf("\r\nSelect test data [1 = AAAAAAAA, 2 = C412C412, 3 = 41414141, 4 = 15151515]: ");
    //         break;
    //     default:
    //         printf("\r\nInvalid value width\n");
    //         // early exit
    // }

    // scanf("%u", &test_data_option);

    // if (test_data_option > 4 || test_data_option < 1)
    // {
    //     printf("\nInvalid option (%u) selected", test_data_option);
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

    // // check addr range 0802 0000 –----- 0803 0000
    // if (start_addr < 0x08020000 || start_addr + width_option > 0x08030000 || end_addr < 0x08020000 || end_addr + width_option > 0x08030000)
    // {
    //     printf("\nInvalid start or end address: must be in 0x0802 0000 to 0x0803 0000 range");
    //     // early exit
    // }

    // // start addr < end addr
    // if (start_addr >= end_addr)
    // {
    //     printf("\nInvalid addresses: start address must be before or equal to end address");
    //     // early exit
    // }


    // // MEMORY TEST
    // for (i = start_addr; i <= end_addr; i += width_option)
    // {
    //     // (start_addr + width_option * i)
    //     switch(width_option) 
    //     {
    //         case 1:
    //             // write
    //             *((char *)(i)) = byte_pattern[test_data_option - 1];
    //             // read
    //             value_from_mem = *((char *)(i));
    //             if ((unsigned char)value_from_mem != byte_pattern[test_data_option - 1]) {
    //                 printf("\nFAIL: value read from memory is %x", (unsigned char) value_from_mem);
    //                 // early exit
    //             }
    //             break;
    //         case 2:
    //             //write
    //             *((unsigned short int *)(i)) = word_pattern[test_data_option - 1];
    //             // read
    //             value_from_mem = *((unsigned short int *)(i));
    //             if ((unsigned short int)value_from_mem != word_pattern[test_data_option - 1]) {
    //                 printf("\nFAIL: value read from memory is %x", (unsigned short int) value_from_mem);
    //                 // early exit
    //             }
    //             break;
    //         case 4:
    //             //write
    //             *((unsigned int *)(i)) = lword_pattern[test_data_option - 1];
    //             // read
    //             value_from_mem = *((unsigned int *)(i));
    //             if ((unsigned int)value_from_mem != lword_pattern[test_data_option - 1]) {
    //                 printf("\nFAIL: value read from memory is %x", (unsigned int) value_from_mem);
    //                 // early exit
    //             }
    //             break;
    //         default:
    //             printf("\r\nInvalid value width\n");
    //             // shouldn't get here, early exit
    //     }

    //     // print progress
    //     if ((i - start_addr) % (1<<10) == 0)
    //     {
    //         printf("\nAddress: %x Data read/written: %x\n", i, (unsigned int) value_from_mem);
    //     }
    // }
}
